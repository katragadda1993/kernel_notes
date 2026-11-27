#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BIT(n) (UINT32_C(1) << (n))
#define FIELD_PREP(mask, value) (((uint32_t)(value) << __builtin_ctz(mask)) & (mask))

struct dma_desc {
    uint64_t dma_addr;
    uint32_t length;
    uint32_t control;
};

struct packed_wire_header {
    uint8_t tag;
    uint32_t value;
} __attribute__((packed));

static const char lesson_marker[]
    __attribute__((used, section(".rodata.lesson"))) = "ELF_SECTION_MARKER";

__attribute__((noinline))
static uint32_t make_control(uint32_t flags, uint32_t length)
{
    const uint32_t length_mask = UINT32_C(0x0000fff0);

    return (flags & UINT32_C(0x0f)) | FIELD_PREP(length_mask, length);
}

int main(void)
{
    struct dma_desc desc = {
        .dma_addr = UINT64_C(0x12345000),
        .length = 4096,
        .control = make_control(BIT(0) | BIT(2), 64),
    };
    struct packed_wire_header wire = {
        .tag = 0xa5,
        .value = UINT32_C(0x11223344),
    };

    printf("pointer_size=%zu\n", sizeof(void *));
    printf("dma_desc_size=%zu addr_off=%zu length_off=%zu control_off=%zu\n",
           sizeof(desc), offsetof(struct dma_desc, dma_addr),
           offsetof(struct dma_desc, length), offsetof(struct dma_desc, control));
    printf("packed_header_size=%zu value_off=%zu\n",
           sizeof(wire), offsetof(struct packed_wire_header, value));
    printf("desc_addr=0x%" PRIx64 " length=%" PRIu32 " control=0x%08" PRIx32 "\n",
           desc.dma_addr, desc.length, desc.control);
    printf("wire_tag=0x%02x wire_value=0x%08" PRIx32 "\n",
           wire.tag, wire.value);
    printf("marker=%s\n", lesson_marker);
    return 0;
}
