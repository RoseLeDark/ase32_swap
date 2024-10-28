#include "esp_partition.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_attr.h"
//#include "esp_private/mmu_psram.h"
//#include "esp_private/esp_mmu.h"
#include <string.h>

#include "esp_private/esp_mmu_map_private.h"
#include "esp_mmu_map.h"

#include <esp_private/mmu_psram_flash.h>
#include <esp_heap_caps.h>

#include "ase32_swap.h"
#include "private/ase32_swap_secure.h"



esp_err_t ase32_sswap_in_page(swap_page_t* page);

#if AASE32_SWAP_PAGE_FAULT_HANGLE == 1
static void IRAM_ATTR handle_page_fault(ase32_swap_t* swap, void* arg,  uint32_t addr) {
    // Betroffene Seite finden
    swap_page_t* page = NULL;
    for (int i = 0; i < swap->page_count; i++) {
        if (swap->pages[i].virt_addr <= addr &&
            swap->pages[i].virt_addr + swap->page_count > addr) {
            page = &swap->pages[i];
            break;
        }
    }

    if (page && !page->is_in_memory) {
        if (ase32_sswap_in_page(page) != ESP_OK) {
            ESP_LOGE(TAG, "Konnte Seite nicht laden: 0x%08x", addr);
        }
    }
} 
#else
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
        page->is_dirty = false;
        page->is_in_memory = true;
        page->last_access = swap->access_counter++;
        page->access_count = 0;

        // MMU-Mapping erstellen
        esp_err_t err = esp_mmu_map(page->phys_addr, swap->config.block_size, MMU_TARGET_PSRAM0, MMU_MEM_CAP_8BIT, 0, (void**)&(page->virt_addr));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "MMU-Mapping fehlgeschlagen: %s", esp_err_to_name(err));
            heap_caps_free((void*)page->phys_addr);
            swap->page_count--;
            return;
        }
    }

    // Seite in Speicher laden, wenn sie nicht bereits geladen ist
    if (page && !page->is_in_memory) {
        if (ase32_sswap_in_page(page) != ESP_OK) {
            ESP_LOGE(TAG, "Konnte Seite nicht laden: 0x%08x", addr);
        }
    }
}
#endif

esp_err_t ase32_swap_init_partition(const ase32_swap_cfg_t cfg, ase32_swap_t* out);
esp_err_t ase32_swap_init_file(const ase32_swap_cfg_t cfg, ase32_swap_t* out);



esp_err_t ase32_swap_init(const ase32_swap_cfg_t cfg, ase32_swap_t* out) {
    if (out == NULL) return ESP_ERR_INVALID_ARG;

    esp_err_t _error;
    out->config = cfg;

    if(out->config.cached_pages < 32) out->config.cached_pages = 32;
    if(out->config.block_size < 512) out->config.block_size = ASE32_SWAP_BLOCK_SIZE_4096;

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
                    ESP_LOGE(TAG, "Fehler beim Schreiben der Seite in die Datei");
                    ret = ESP_ERR_NO_MEM;
                }
                fflush(swap->file);
            } else {
                // In Partition schreiben
                ret = esp_partition_write(swap->partition, offset, data, swap->config.block_size);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Fehler beim Schreiben der Seite in die Partition: %s", esp_err_to_name(ret));
                }
            }

            if (ret == ESP_OK) {
                page->is_dirty = false;
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
            page->is_in_memory = false;
            page->is_dirty = false;
            page->is_used = false;

            // Zugriffszähler und letzte Zugriffszeit zurücksetzen
            page->access_count = 0;
            page->last_access = 0;

            // Optional: Debug-Informationen ausgeben
            if (ase32_swap_debug_trace) {
                ase32_swap_debug_trace(ESP_LOG_INFO, "Seite freigegeben: 0x%08x", ptr);
            }

            return;
        }
    }

    // Optional: Warnung ausgeben, wenn keine passende Seite gefunden wurde
    if (ase32_swap_debug_trace) {
        ase32_swap_debug_trace(ESP_LOG_WARN, "Keine Seite gefunden für: 0x%08x", ptr);
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
        page->is_dirty = false;
        page->is_in_memory = true;
        page->last_access = swap->access_counter++;
        page->access_count = 0;
        page->is_used = true;  // Hier setzen wir is_used auf true

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
            page->is_dirty = true;
        }
    }
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
        ESP_LOGE(TAG, "Konnte Datei nicht öffnen: %s", cfg.storage_name);
        return ESP_ERR_NOT_FOUND;
    }

    // Dateigröße ermitteln
    fseek(out->file, 0, SEEK_END);
    out->physic_size = ftell(out->file);
    fseek(out->file, 0, SEEK_SET);

    // Maximal mögliche Seiten berechnen
    out->config.max_pages = out->physic_size / cfg.block_size;

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

void ase32_swap_dump(const ase32_swap_t* swap) {
    if (swap == NULL) return;

    printf("Swap System Dump:\n");
    printf("Total Pages: %d\n", swap->page_count);
    printf("Max Pages: %d\n", swap->config.max_pages);
    printf("Block Size: %d\n", swap->config.block_size);
    printf("Cached Pages: %d\n\n", swap->config.cached_pages);

    for (uint32_t i = 0; i < swap->page_count; i++) {
        const swap_page_t* page = &swap->pages[i];
        printf("Page %d:\n", i);
        printf("  Virt Addr: 0x%08x\n", page->virt_addr);
        printf("  Phys Addr: 0x%08x\n", page->phys_addr);
        printf("  Partition Offset: %d\n", page->partition_offset);
        printf("  Dirty: %s\n", page->is_dirty ? "Yes" : "No");
        printf("  In Memory: %s\n", page->is_in_memory ? "Yes" : "No");
        printf("  Last Access: %d\n", page->last_access);
        printf("  Access Count: %d\n", page->access_count);
        printf("  Used: %s\n\n", page->is_used ? "Yes" : "No");
    }
}