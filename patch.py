with open(r'd:\Signatures_OS\tools\image_builder.c', 'r') as f:
    text = f.read()

# Change dir array size from 18 to 20
text = text.replace('FAT32_DirEntry dir[18];', 'FAT32_DirEntry dir[20];')

sz_add = '''    FILE* f_boot1 = fopen(" boot_sound/bootsound1.wav\, \rb\);
 uint32_t boot1_sz = 0;
 if (f_boot1) { fseek(f_boot1, 0, SEEK_END); boot1_sz = ftell(f_boot1); fseek(f_boot1, 0, SEEK_SET); }

 FILE* f_doom_elf = fopen(\build/doom.elf\, \rb\);
 uint32_t doom_elf_sz = 0;
 if (f_doom_elf) { fseek(f_doom_elf, 0, SEEK_END); doom_elf_sz = ftell(f_doom_elf); fseek(f_doom_elf, 0, SEEK_SET); }

 FILE* f_doom_wad = fopen(\assets/doom/DOOM1.WAD\, \rb\);
 uint32_t doom_wad_sz = 0;
 if (f_doom_wad) { fseek(f_doom_wad, 0, SEEK_END); doom_wad_sz = ftell(f_doom_wad); fseek(f_doom_wad, 0, SEEK_SET); }
'''
text = text.replace(' FILE* f_boot1 = fopen(\boot_sound/bootsound1.wav\, \rb\);\n uint32_t boot1_sz = 0;\n if (f_boot1) { fseek(f_boot1, 0, SEEK_END); boot1_sz = ftell(f_boot1); fseek(f_boot1, 0, SEEK_SET); }\n', sz_add)

dir_add = ''' memcpy(dir[17].name, \BOOT1 WAV\, 11);
 dir[17].attr = 0x20;
 dir[17].fst_clus_lo = next_cluster;
 dir[17].file_size = boot1_sz;
 next_cluster = allocate_clusters(fat, next_cluster, dir[17].file_size, bytes_per_cluster);

 memcpy(dir[18].name, \DOOM ELF\, 11);
 dir[18].attr = 0x20;
 dir[18].fst_clus_lo = next_cluster;
 dir[18].file_size = doom_elf_sz;
 next_cluster = allocate_clusters(fat, next_cluster, dir[18].file_size, bytes_per_cluster);

 memcpy(dir[19].name, \DOOM1 WAD\, 11);
 dir[19].attr = 0x20;
 dir[19].fst_clus_lo = next_cluster;
 dir[19].file_size = doom_wad_sz;
 next_cluster = allocate_clusters(fat, next_cluster, dir[19].file_size, bytes_per_cluster);
'''
text = text.replace(' memcpy(dir[17].name, \BOOT1 WAV\, 11);\n dir[17].attr = 0x20;\n dir[17].fst_clus_lo = next_cluster;\n dir[17].file_size = boot1_sz;\n next_cluster = allocate_clusters(fat, next_cluster, dir[17].file_size, bytes_per_cluster);\n', dir_add)

write_add = ''' // BOOT1.WAV
 if (f_boot1) {
 if (boot1_sz > 0) {
 uint8_t* boot1_buf = malloc(boot1_sz);
 fread(boot1_buf, 1, boot1_sz, f_boot1);
 fseek(img, (data_lba_base + (dir[17].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
 fwrite(boot1_buf, 1, boot1_sz, img);
 free(boot1_buf);
 }
 fclose(f_boot1);
 }

 // DOOM.ELF
 if (f_doom_elf) {
 if (doom_elf_sz > 0) {
 uint8_t* buf = malloc(doom_elf_sz);
 fread(buf, 1, doom_elf_sz, f_doom_elf);
 fseek(img, (data_lba_base + (dir[18].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
 fwrite(buf, 1, doom_elf_sz, img);
 free(buf);
 }
 fclose(f_doom_elf);
 }

 // DOOM1.WAD
 if (f_doom_wad) {
 if (doom_wad_sz > 0) {
 uint8_t* buf = malloc(doom_wad_sz);
 fread(buf, 1, doom_wad_sz, f_doom_wad);
 fseek(img, (data_lba_base + (dir[19].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
 fwrite(buf, 1, doom_wad_sz, img);
 free(buf);
 }
 fclose(f_doom_wad);
 }
'''
text = text.replace(''' // BOOT1.WAV
 if (f_boot1) {
 if (boot1_sz > 0) {
 uint8_t* boot1_buf = malloc(boot1_sz);
 fread(boot1_buf, 1, boot1_sz, f_boot1);
 fseek(img, (data_lba_base + (dir[17].fst_clus_lo * bpb.sectors_per_cluster)) * SECTOR_SIZE, SEEK_SET);
 fwrite(boot1_buf, 1, boot1_sz, img);
 free(boot1_buf);
 }
 fclose(f_boot1);
 }''', write_add)

with open(r'd:\Signatures_OS\tools\image_builder.c', 'w') as f:
 f.write(text)
