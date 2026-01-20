ASM= nasm
src_dir= src
build_dir= Build

$(build_dir)/main_floppy.img : $(build_dir)/main.bin
	cp $(build_dir)/main.bin $(build_dir)/main_floppy.img
	truncate -s 1440k $(build_dir)/main_floppy.img

$(build_dir)/main.bin : $(src_dir)/main.asm 
	$(ASM) $(src_dir)/main.asm -f bin -o $(build_dir)/main.bin
