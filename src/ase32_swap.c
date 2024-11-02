#include "ase32_swap.h"
#include "ase32_swap_crypt.h"
#include "ase32_swap_compress.h"
#include "ase32_swap_debug.h"

#include "esp_partition.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_attr.h"
#include <string.h>

#include "esp_private/esp_mmu_map_private.h"
#include "esp_mmu_map.h"

#include <esp_private/mmu_psram_flash.h>
#include <esp_heap_caps.h>

#include "esp_timer.h"

static void IRAM_ATTR handle_page_fault(ase32_swap_t* swap, void* arg, uint32_t addr) {
    // Betroffene Seite finden
    swap_page_t* page = NULL;
    for (int i = 0; i < swap->page_count; i++) {
        if (swap->pages[i].virt_addr <= addr && swap->pages[i].virt_addr + swap->config.block_size > addr) {
            page = &swap->pages[i];
            break;
        }
    }

    // Neue Seite allokieren, wenn keine Seite gefunden wird
    if (!page) {
        if (swap->page_count >= swap->config.max_pages) {
            if(ase32_swap_debug_trace) 
                ase32_swap_debug_trace(ESP_LOG_ERROR,  "Keine freien Seiten mehr verfügbar");
            return;
        }

        page = &swap->pages[swap->page_count++];
        page->virt_addr = addr & ~(swap->config.block_size - 1); // Align to block size
        page->phys_addr = (uint32_t)heap_caps_malloc(swap->config.block_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!page->phys_addr) {
            if(ase32_swap_debug_trace) 
                ase32_swap_debug_trace(ESP_LOG_ERROR,  "Speicherallokation für neue Seite fehlgeschlagen");
            swap->page_count--;
            return;
        }

        page->partition_offset = swap->page_count * swap->config.block_size;
        page->is_dirty = 0;
        page->is_in_memory = 1;
        page->last_access = swap->access_counter++;
        page->access_count = 0;

        // MMU-Mapping erstellen
        esp_err_t err = esp_mmu_map(page->phys_addr, swap->config.block_size, MMU_TARGET_PSRAM0, MMU_MEM_CAP_8BIT, 0, (void**)&(page->virt_addr));
        if (err != ESP_OK) {
            ESP_LOGE("swap", "MMU-Mapping fehlgeschlagen: %s", esp_err_to_name(err));
            heap_caps_free((void*)page->phys_addr);
            swap->page_count--;
            return;
        }
    }

    // Seite in Speicher laden, wenn sie nicht bereits geladen ist
    if (page && !page->is_in_memory) {
        if (ase32_sswap_in_page(page) != ESP_OK) {
            ESP_LOGE("swap", "Konnte Seite nicht laden: 0x%08x", addr);
        }
    }
}

esp_err_t ase32_swap_init_partition(const ase32_swap_cfg_t cfg, ase32_swap_t* out);
esp_err_t ase32_swap_init_file(const ase32_swap_cfg_t cfg, ase32_swap_t* out);



esp_err_t ase32_swap_init(const ase32_swap_cfg_t cfg, ase32_swap_t* out) {
    if (out == NULL) return ESP_ERR_INVALID_ARG;

    esp_err_t _error;
    out->config = cfg;

    if(out->config.cached_pages < 32) out->config.cached_pages = 32;
    if(out->config.block_size < 32) out->config.block_size = ASE32_SWAP_BLOCK_SIZE_4096;

    out->pages = (swap_page_t*)heap_caps_malloc(sizeof(swap_page_t) * cfg.max_pages, MALLOC_CAP_8BIT);
    if (out->pages == NULL) {
        if(ase32_swap_debug_trace) 
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Speicherallokation für Seiten fehlgeschlagen");
        return ESP_ERR_NO_MEM;
    }

    if (cfg.storage_type == ASE32_SWAP_STORAGE_PARTITION) {
        _error = ase32_swap_init_partition(cfg, out);
    } else {
        _error = ase32_swap_init_file(cfg, out);
    }

    if (_error == ESP_OK) {
        
    } else {
        free(out->pages);  // Freigeben des Seiten-Speichers im Fehlerfall
    }

    return _error;
}

void ase32_swap_deinit(ase32_swap_t* swap) {
    if (swap == NULL) return;

    // Speicher freigeben, der für Seiten allokiert wurde
    for (uint32_t i = 0; i < swap->page_count; i++) {
        if (swap->pages[i].phys_addr) {
            heap_caps_free((void*)swap->pages[i].phys_addr);
        }
    }

    // Cache-Puffer freigeben
    if (swap->cache_buffer) {
        heap_caps_free(swap->cache_buffer);
    }

    // Datei schließen, wenn eine Datei verwendet wurde
    if (swap->file) {
        fclose(swap->file);
    }

    // Speicher für Seiten freigeben
    if (swap->pages) {
        heap_caps_free(swap->pages);
    }

    // Optionale Debug-Ausgabe
    if (ase32_swap_debug_trace) {
        ase32_swap_debug_trace(ESP_LOG_INFO, "Swap-Speicher erfolgreich freigegeben");
    }
}

esp_err_t ase32_swap_flush(ase32_swap_t* swap) {
    if (swap == NULL) return ESP_ERR_INVALID_ARG;

    esp_err_t ret = ESP_OK;

    for (uint32_t i = 0; i < swap->page_count; i++) {
        swap_page_t* page = &swap->pages[i];
        if (page->is_dirty) {
            // Schreibe die Seite zurück in die Partition oder Datei
            size_t offset = page->partition_offset;
            void* data = (void*)page->phys_addr;

            if (swap->file) {
                // In Datei schreiben
                fseek(swap->file, offset, SEEK_SET);
                if (fwrite(data, 1, swap->config.block_size, swap->file) != swap->config.block_size) {
                    ESP_LOGE("swap", "Fehler beim Schreiben der Seite in die Datei");
                    ret = ESP_ERR_NO_MEM;
                }
                fflush(swap->file);
            } else {
                // In Partition schreiben
                ret = esp_partition_write(swap->partition, offset, data, swap->config.block_size);
                if (ret != ESP_OK) {
                    ESP_LOGE("swap", "Fehler beim Schreiben der Seite in die Partition: %s", esp_err_to_name(ret));
                }
            }

            if (ret == ESP_OK) {
                page->is_dirty = 0;
            }
        }
    }

    return ret;
}

void ase32_swap_free(ase32_swap_t* swap, void* ptr) {
    if (swap == NULL || ptr == NULL) return;

    // Finde die Seite, die zu 'ptr' gehört
    for (uint32_t i = 0; i < swap->page_count; i++) {
        swap_page_t* page = &swap->pages[i];
        if (page->is_used && (void*)page->virt_addr <= ptr && ptr < (void*)(page->virt_addr + swap->config.block_size)) {
            // Physikalischen Speicher der Seite freigeben
            if (page->phys_addr) {
                heap_caps_free((void*)page->phys_addr);
                page->phys_addr = 0;
            }
            // Virtuelle Adresse ungültig machen
            page->virt_addr = 0;
            page->is_in_memory = 0;
            page->is_dirty = 0;
            page->is_used = 0;

            // Zugriffszähler und letzte Zugriffszeit zurücksetzen
            page->access_count = 0;
            page->last_access = 0;

            // Optional: Debug-Informationen ausgeben
            if (ase32_swap_debug_trace) {
                ase32_swap_debug_trace(ESP_LOG_INFO, "Seite freigegeben");
            }

            return;
        }
    }

    // Optional: Warnung ausgeben, wenn keine passende Seite gefunden wurde
    if (ase32_swap_debug_trace) {
        ase32_swap_debug_trace(ESP_LOG_WARN, "Keine Seite gefunden");
    }
}

// Speicher allokieren mit Swap-Support
void* ase32_swap_malloc(ase32_swap_t* swap, size_t size) {
    if (size == 0 || size > (swap->config.max_pages * swap->config.block_size)) {
        return NULL;
    }

    uint32_t pages_needed = (size + swap->config.block_size - 1) / swap->config.block_size;
    uint32_t virt_addr = (uint32_t)heap_caps_malloc(size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

    if (!virt_addr) {
        if (ase32_swap_debug_trace) 
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Virtuelle Speicher-Allokation fehlgeschlagen");
        return NULL;
    }

    // Physikalischen Speicher allokieren und Seiten registrieren
    for (uint32_t i = 0; i < pages_needed; i++) {
        swap_page_t* page = &swap->pages[swap->page_count++];
        page->virt_addr = virt_addr + (i * swap->config.block_size);
        page->phys_addr = (uint32_t)heap_caps_malloc(swap->config.block_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        page->partition_offset = i * swap->config.block_size;
        page->is_dirty = 0;
        page->is_in_memory = 1;
        page->last_access = swap->access_counter++;
        page->access_count = 0;
        page->timestamp = esp_timer_get_time();
        page->is_used = 1;  // Hier setzen wir is_used auf 1

        if (!page->phys_addr) {
            // Wenn nicht genug RAM, frühere Seiten freigeben
            while (i > 0) {
                i--;
                heap_caps_free((void*)swap->pages[--swap->page_count].phys_addr);
            }
            heap_caps_free((void*)virt_addr);
            return NULL;
        }

        // MMU-Mapping erstellen
        esp_err_t err = esp_mmu_map(page->phys_addr, swap->config.block_size, MMU_TARGET_PSRAM0, 
                                    MMU_MEM_CAP_8BIT | MMU_MEM_CAP_READ | MMU_MEM_CAP_WRITE, 0, 
                                    (void**)&(page->virt_addr));
        if (err != ESP_OK) {
            if (ase32_swap_debug_trace) 
                ase32_swap_debug_trace(ESP_LOG_ERROR, "MMU-Mapping fehlgeschlagen");
            while (i > 0) {
                i--;
                heap_caps_free((void*)swap->pages[--swap->page_count].phys_addr);
            }
            heap_caps_free((void*)virt_addr);
            return NULL;
        }
    }

    return (void*)virt_addr;
}
void ase32_swap_mark_dirty(ase32_swap_t* swap, void* ptr, size_t size) {
    uint32_t start_addr = (uint32_t)ptr;
    uint32_t end_addr = start_addr + size;

    for (int i = 0; i < swap->page_count; i++) {
        swap_page_t* page = &swap->pages[i];
        if (page->virt_addr + swap->config.block_size > start_addr &&
            page->virt_addr < end_addr) {
            page->is_dirty = 1;
            page->timestamp = esp_timer_get_time();
            page->access_count++;
        }
    }
}

void* ase32_swap_memcpy(ase32_swap_t* swap, void* dest, const void* src, size_t s ) {
    bool is_src = 0, is_dest = 0;

    for (int i = 0; i < swap->page_count; i++) { 
        swap_page_t* page = &swap->pages[i];
        if (  page->virt_addr == dest) {
            page->last_access++;
            page->access_count++;
            page->timestamp = esp_timer_get_time();
            is_dest = 1;
        } else if( page->virt_addr == src) {
            page->last_access++;
            page->access_count++;
            page->timestamp = esp_timer_get_time();
            is_src = 1;
        }

        if(is_src && is_dest) 
            break;
    }
    return memcpy(dest, src, s);
}

esp_err_t ase32_swap_init_partition(const ase32_swap_cfg_t cfg, ase32_swap_t* out) {
    if (out == NULL) return ESP_ERR_INVALID_ARG;
    esp_err_t ret;

     // Swap-Partition finden
    out->partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_SWAP, cfg.storage_name);
    if (out->partition == NULL) {
        if(ase32_swap_debug_trace) 
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Keine Swap-Partition gefunden");
        return ESP_ERR_NOT_FOUND;
    }

    // Maximal mögliche Seiten berechnen
    out->physic_size = out->partition->size;
    out->config.max_pages = out->physic_size / cfg.block_size;

    if (out->config.cached_pages > out->config.max_pages) {
        ESP_LOGW("MMU_CACHE", "Adjusting num_pages from %d to %d based on partition size",
                 out->config.cached_pages, out->config.max_pages);
        out->config.cached_pages = out->config.max_pages;
    }

     // Initialize partition header if needed
    uint32_t magic;
    esp_err_t err = esp_partition_read(out->partition, 0, &magic, sizeof(magic));
    if (err != ESP_OK) {
        return err;
    }

     if (magic != ASE32_SWAP_PARTITION_MAGIC) {
        // Initialize partition
        magic = ASE32_SWAP_PARTITION_MAGIC;
        err = esp_partition_erase_range(out->partition, 0, out->physic_size);
        if (err != ESP_OK) {
            return err;
        }
       
        err = esp_partition_write(out->partition, 0, &magic, sizeof(magic));
        if (err != ESP_OK) {
            return err;
        }
    }


    // Cache-Puffer allokieren
    out->cache_size = (uint32_t)cfg.block_size * cfg.cached_pages; //  SWAP_PAGE_SIZE * CACHE_PAGES;
    out->cache_buffer = heap_caps_malloc(out->cache_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

    if (out->cache_buffer == NULL) {
        if(ase32_swap_debug_trace) 
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Cache-Allokation fehlgeschlagen");
        return ESP_ERR_NO_MEM;
    }

    // MMU-Handler registrieren
    ret = ase32_swap_set_page_fault_handler(handle_page_fault, NULL);
    if (ret != ESP_OK) {
        if(ase32_swap_debug_trace) 
            ase32_swap_debug_trace(ESP_LOG_ERROR, "MMU Handler Registrierung fehlgeschlagen");
        free(out->cache_buffer);
        return ret;
    }

    out->page_count = 0;
    out->access_counter = 0;

    return ESP_OK;
}

esp_err_t ase32_swap_init_file(const ase32_swap_cfg_t cfg, ase32_swap_t* out)  {
    if (out == NULL || out->file == NULL) return ESP_ERR_INVALID_ARG;
    esp_err_t ret;

    out->file = fopen(cfg.storage_name, "rb+");
    if (out->file == NULL) {
        ESP_LOGE("swap", "Konnte Datei nicht öffnen: %s", cfg.storage_name);
        return ESP_ERR_NOT_FOUND;
    }

    // Dateigröße ermitteln
    fseek(out->file, 0, SEEK_END);
    out->physic_size = ftell(out->file);
    fseek(out->file, 0, SEEK_SET);

    // Maximal mögliche Seiten berechnen
    out->config.max_pages = out->physic_size / cfg.block_size;

    if (out->config.cached_pages > out->config.max_pages) {
        ESP_LOGW("MMU_CACHE", "Adjusting num_pages from %d to %d based on partition size",
                 out->config.cached_pages, out->config.max_pages);
        out->config.cached_pages = out->config.max_pages;
    }

    // Cache-Puffer allokieren
    out->cache_size = (uint32_t)cfg.block_size * cfg.cached_pages;
    out->cache_buffer = heap_caps_malloc(out->cache_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

    if (out->cache_buffer == NULL) {
        if (ase32_swap_debug_trace)
            ase32_swap_debug_trace(ESP_LOG_ERROR, "Cache-Allokation fehlgeschlagen");
        fclose(out->file);
        return ESP_ERR_NO_MEM;
    }

    // MMU-Handler registrieren
    ret = ase32_swap_set_page_fault_handler(handle_page_fault, NULL);
    if (ret != ESP_OK) {
        if (ase32_swap_debug_trace)
            ase32_swap_debug_trace(ESP_LOG_ERROR, "MMU Handler Registrierung fehlgeschlagen");
        fclose(out->file);
        free(out->cache_buffer);
        return ret;
    }

    out->page_count = 0;
    out->access_counter = 0;
    return ESP_OK;
}


#include "ase32_swap_mmu.h"

//Schreibt eine Seite in die Partition oder Datei
esp_err_t ase32_swap_out_page(ase32_swap_t* swap, swap_page_t* page) {
    if(page->is_used == 1) return ESP_ERR_INVALID_STATE;

    if (!page->is_dirty || !page->is_in_memory) {
        return ESP_OK;
    }

    // Temporärer Puffer für Kompression
    void* temp_buffer = heap_caps_malloc(swap->config.block_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!temp_buffer) {
        return ESP_ERR_NO_MEM;
    }

    size_t compressed_size = swap->config.block_size;
    esp_err_t ret = ase32_compress_page(swap, (void*)page->phys_addr, temp_buffer, &compressed_size);
    if(ret != ESP_OK) return ret;

    
    // Verschlüssele die komprimierten Daten
    ret = ase32_encrypt_page(swap, temp_buffer, compressed_size);
    if(ret != ESP_OK) return ret;

    if(swap->config.storage_type == ASE32_SWAP_STORAGE_FILE && swap->file) {
        fseek(swap->file, page->partition_offset, SEEK_SET);
        // Schreibe zuerst die komprimierte Größe
        fwrite(&compressed_size, sizeof(size_t), 1, swap->file);
        // Dann die Daten
        size_t written = fwrite(temp_buffer, 1, compressed_size, swap->file);
        if (written != compressed_size) {
            ret = ESP_FAIL;
        }
        fflush(swap->file);
    } else if (swap->config.storage_type == ASE32_SWAP_STORAGE_PARTITION && swap->partition) {
        // Schreibe zuerst die komprimierte Größe
        ret = esp_partition_write(swap->partition, page->partition_offset, 
                                &compressed_size, sizeof(size_t));
        if(ret != ESP_OK) return ret;

        // Dann die Daten
        ret = esp_partition_write(swap->partition, 
                                page->partition_offset + sizeof(size_t),
                                temp_buffer, compressed_size);
        if(ret != ESP_OK) return ret;
        
    }
    
    heap_caps_free(temp_buffer);

    /*page->is_dirty = 0;
    heap_caps_free((void*)page->phys_addr);
    page->phys_addr = 0;
    page->is_in_memory = 0;*/

    ret = ase32_swap_unregist_page(page);

    
    return ret;
}



//Lädt eine Seite aus der Partition oder Datei
esp_err_t ase32_swap_in_page(ase32_swap_t* swap, swap_page_t* page) {
    if (page->is_in_memory) {
        return ESP_OK;
    }

    if (!page->phys_addr) {
        page->phys_addr = (uint32_t)heap_caps_malloc(swap->config.block_size, 
                                                    MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (!page->phys_addr) {
            return ESP_ERR_NO_MEM;
        }
    }

    // Temporärer Puffer für komprimierte Daten
    void* temp_buffer = heap_caps_malloc(swap->config.block_size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!temp_buffer) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    size_t compressed_size = 0;

    // Lese die komprimierte Größe
    if (swap->file) {
        fseek(swap->file, page->partition_offset, SEEK_SET);
        if (fread(&compressed_size, sizeof(size_t), 1, swap->file) != 1) {
            ret = ESP_FAIL;
        } else {
            // Lese die komprimierten Daten
            if (fread(temp_buffer, 1, compressed_size, swap->file) != compressed_size) {
                ret = ESP_FAIL;
            }
        }
    } else if (swap->partition) {
        ret = esp_partition_read(swap->partition, page->partition_offset, 
                               &compressed_size, sizeof(size_t));
        if(ret != ESP_OK) return ret;
        
        ret = esp_partition_read(swap->partition, 
                                page->partition_offset + sizeof(size_t),
                                temp_buffer, compressed_size);
        if(ret != ESP_OK) return ret;

    }

    // Entschlüssele die Daten
    ret = ase32_decrypt_page(swap, temp_buffer, compressed_size);
    if(ret != ESP_OK) return ret;

    // Dekomprimiere die Daten
    ret = ase32_decompress_page(swap, temp_buffer, compressed_size, (void*)page->phys_addr);
    if(ret != ESP_OK) return ret;


    heap_caps_free(temp_buffer);

    // Aktualisiere MMU-Mapping
    ret = ase32_swap_register_page(swap, page);

    /*ret = esp_mmu_map(page->phys_addr, swap->config.block_size, 
                        MMU_TARGET_PSRAM0, 
                        MMU_MEM_CAP_8BIT | MMU_MEM_CAP_READ | MMU_MEM_CAP_WRITE, 
                        0, (void**)&(page->virt_addr));
    if(ret != ESP_OK) return ret;
        

    page->is_in_memory = 1;
    page->last_access = swap->access_counter++;
    page->access_count++;*/

    

    return ret;

}

void ase32_swap_defragment(ase32_swap_t* swap, ase32_swap_defrag_callback callback) {
    size_t total_moves = 0;
    size_t completed_moves = 0;
    
    // Zähle notwendige Bewegungen
    for (size_t i = 0; i < swap->page_count; i++) {
        if (!swap->pages[i].is_used && i < swap->page_count - 1) {
            total_moves++;
        }
    }
    
    // Defragmentierung durchführen
    size_t write_pos = 0;
    for (size_t read_pos = 0; read_pos < swap->page_count; read_pos++) {
        if (swap->pages[read_pos].is_used) {
            if (write_pos != read_pos) {
                memcpy(&swap->pages[write_pos], &swap->pages[read_pos], sizeof(swap_page_t));
                swap->pages[write_pos].phys_addr = write_pos * sizeof(swap_page_t);
                completed_moves++;
                
                if (callback) {
                    callback((float)completed_moves / total_moves);
                }
            }
            write_pos++;
        }
    }
    
    swap->page_count = write_pos;
}