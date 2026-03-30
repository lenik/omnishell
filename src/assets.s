.section .assets,"a"
.align 4                 /* Ensures 4-byte alignment */
.global omni_assets_start
omni_assets_start:
    .incbin "assets.zip"
.global omni_assets_end
omni_assets_end:
