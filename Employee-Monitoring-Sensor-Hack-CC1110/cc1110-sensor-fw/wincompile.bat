sdcc --out-fmt-ihx --code-loc 0x000 --code-size 0x8000 --xram-loc 0xf000 --xram-size 0x300 --iram-size 0x100 --model-small --opt-code-speed sensor-main.c
packihx sensor-main.ihx > sensor-main.hex