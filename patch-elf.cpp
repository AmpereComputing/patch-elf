// Copyright (c) 2023, Ampere Computing LLC
//
// SPDX-License-Identifier: BSD-3-Clause
//
// Ampere Computing LLC license terms can be found in LICENSE.txt
// in the root directory.
// 
// patch-elf.cpp
// This utility program parses ELF code section headers for the SHF_EXECINTR attribute,
// then overlays code segment instruction data read from the kcore kernel image file
// created by 'perf report --kcore'.
//

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>

#define DEBUG		0
#define ENABLE_PATCH	1

using namespace ELFIO;

bool b_verbose = false;

// Utility code
const char *machine_name(Elf_Half e_machine);
const char *type_name(Elf_Half e_type);
char *strip_path(char *path);

bool patch(elfio& reader, elfio& writer);

// Very simplistic 
// If there's a i'-h' or '--help' argument, ignore everything else.
// If there's a '-v' or ''--verbose' argument, turn it on. The required
// kcore_file and vmlinux file arguments are otherwise required and
// positional.
int parseargs(int argc, char** argv)
{
	int n_argc = argc;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			std::cout << "\npatch-elf overlays the kernel image from a local copy of\n";
			std::cout << "\'/proc/kcore\' to the  corresponding (by address) ELF sections\n"
			             "in a local copy of the vmlinux ELF file.\n";
			std::cout << "A local \'/proc/kcore\' is created by:\n";
			std::cout << "    \'perf report --kcore -e cs_etm/@tmc_etr1/k ...\'\n";
			std::cout << "The local (patched) vmlinux copy is used by:\n";
			std::cout << "    \'perf script -s arm-cs-trace-disasm.py ...\'\n";
			std::cout << "See the CoreSight Hardware-Assisted Trace Application Note for\n"
				     "use of the \'perf report\' and \'perf script\' commands.\n\n";
			return 1;
		}
		else if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v"))
		{
			b_verbose = true;
			n_argc--;
		}

	}
	return (n_argc != 3) ? 1 : 0;
}

void print_elf_headers(elfio &file, const char *filename)
{
	//
	// Print ELF file program header info
	std::cout << "ELF File " <<  filename <<" Properties" << std::endl;
	// Print ELF file properties
	std::cout << "ELF file class:     ";

	if (file.get_class() == ELFCLASS32 )
		std::cout << "ELF32" << std::endl;
	else
		std::cout << "ELF64" << std::endl;
	std::cout << "ELF file encoding:  ";
	if (file.get_encoding() == ELFDATA2LSB)
		std::cout << "Little endian" << std::endl;
	else
		std::cout << "Big endian" << std::endl;
	std::cout << "Machine:            ";
	std::cout << machine_name(file.get_machine()) << std::endl;
	std::cout << "Type:               ";
	std::cout << type_name(file.get_type()) << std::endl;
	//
	// Print ELF file segments info
	Elf_Half seg_num = file.segments.size();
	std::cout << "Number of segments: " << seg_num << std::endl;
	for ( int i = 0; i < seg_num; ++i ) {
		segment* p_seg = file.segments[i];
		if (b_verbose) {
			std::cout << std::dec
	       	        	<< " [" << std::setfill(' ') << std::setw(2) << i << "] 0x" 
				<< std::hex
				<< std::setfill(' ')
				<< p_seg->get_flags()
				<< std::setw(20)
				<< "0x"
				<< p_seg->get_virtual_address()
				<< std::setw(10)
				<< "0x"
				<< p_seg->get_file_size()
				<< std::setw(10)
				<< "0x"
				<< std::setfill(' ')
				<< p_seg->get_memory_size()
				<< std::endl;
		}
	} // end for

	//
	// Print ELF file sections info
	Elf_Half sec_num = file.sections.size();
	std::cout << "Number of sections: " << sec_num << std::endl;
	std::cout << "Executable sections:" << std::endl;
	for ( int i = 0; i < sec_num; ++i ) {
		section* p_sec = file.sections[i];
		if ((p_sec->get_type() & SHT_PROGBITS) && (p_sec->get_flags() & SHF_EXECINSTR)) {
			if (b_verbose) {
				std::cout << std::dec
					<< " [" << std::setfill(' ') << std::setw(2) << i << "] "
					<< std::hex
					<< std::setw(24)
					<< p_sec->get_name()
					<< std::setfill(' ')
					<< std::setw(10)
					<< p_sec->get_size()
					<< std::endl;
			}
		}
	} // end for
}

int main(int argc, char** argv)
{
	bool b_patched = false;

	switch(parseargs(argc, argv))
	{
		case 0:
			break;
		case 1:
			std::cout << "Usage: " << strip_path(argv[0])
				  << " <--verbose> kcore_file vmlinux_file"
				  << std::endl;
			return 1;
	}

	// Create elfio reader and writer
	elfio reader;
	elfio writer;

	// Load kcore ELF data
	if ( !reader.load((b_verbose) ? argv[2] : argv[1])) {
		std::cout << "Can't find or process kcore ELF file " << argv[1] << std::endl;
		return 1;
	}
	// Write vmlinux ELF data
	if ( !writer.load((b_verbose) ? argv[3] :  argv[2])) {
		std::cout << "Can't find or process vmlinux ELF file " << argv[2] << std::endl;
		return 1;
	}

	print_elf_headers(reader, strip_path((b_verbose) ? argv[2] : argv[1])); 
	print_elf_headers(writer, strip_path((b_verbose) ? argv[3] : argv[2])); 

	if ((reader.get_class() != ELFCLASS64) || (writer.get_class() != ELFCLASS64)) {
		std::cout << "Files are incompatible - ELFCLASS" << std::endl;
		return 1;
	}

	b_patched = patch(reader, writer);

	// save function appears to shorten the file by 4 bytes. I've found no problem
	// in the patch code. The section headers (e.g. section size) may/may not get
	// recalculated during the write.
#	if ENABLE_PATCH
	if (b_patched)
		writer.save((b_verbose) ? argv[3] : argv[2]);
#	endif 	// ENABLE_PATCH

	return 0;
}

bool patch(elfio& reader, elfio& writer)
{
	// Print ELF file sections info
	Elf_Half sec_num = writer.sections.size();
	bool b_patched = false;

	for (int i = 0; i < sec_num; ++i) {
		section* p_sec = writer.sections[i];

		Elf64_Addr section_addr = p_sec->get_address();
		Elf_Xword section_sz = p_sec->get_size();

		if ((p_sec->get_type() & SHT_PROGBITS) && (p_sec->get_flags() & SHF_EXECINSTR))
		{
			Elf_Half seg_num = reader.segments.size();
			Elf64_Addr segment_addr = 0;
			Elf64_Addr segment_end = 0;
			Elf64_Off segment_sz = 0;
			Elf64_Off left = 0;
			Elf64_Off offset  = 0;

			const char *p_dest = nullptr;

			if (b_verbose)	{
				std::cout <<  "----------------"
					  << std::endl
					  << "section[" << std::dec << std::setw(2) << i << "]  "
					  << std::hex
					  << std::setw(8)
					  << section_addr
					  << "    size     "
					  << std::setw(8)
					  << section_sz
					  << std::endl;
			}
			for (int j = 0; j < seg_num; ++j) { 
				segment* p_seg = reader.segments[j];
				const char *p_src  = nullptr;

				// note file size may be less than memory size
				segment_sz = p_seg->get_file_size();

				segment_addr = p_seg->get_virtual_address();
				segment_end = segment_addr + segment_sz;

				// adjust section address if not complete
				section_addr += (left)? section_sz - left : 0;

				// section possibly overlaps segments?
				if ((section_addr >= segment_addr) &&
				    (section_addr < segment_end)) {
					// offset of segment address to section
					offset = section_addr - segment_addr;	// align? 
					if (b_verbose) {
						std::cout << "segment[" << std::dec << std::setw(2) << j << "]  "
							  << std::hex
							  << std::setw(8)
							  << segment_addr
							  << "    size     "
					  		  << std::setw(8)
							  << segment_sz
							  << std::endl << std::endl;
					}
					p_dest = p_sec->get_data() + ((left) ? section_sz - left : 0);
					p_src  = p_seg->get_data() + offset;

					if (left)
						section_sz = left;
					if (section_sz > (segment_sz - offset))
						left = section_sz - (segment_sz - offset);
					else
						left = 0;
#					if DEBUG	// DEBUG
					printf("p_dest            %0lx\n", (uint64_t)p_dest);
					printf("p_src             %0lx\n", (uint64_t)p_src);
					std::cout << " dest addr        "
						  << std::hex
					  	  << section_addr
					  	  << std::endl;
					std::cout << " src addr         "
						  << std::hex
						  << segment_addr + offset
						  << std::endl;
					std::cout << " segment offset   "
						  << std::hex
						  << offset
						  << std::endl;
					std::cout << " section overlap  "
						  << std::hex
						  << left
						  << std::endl;
#					endif		// DEBUG
					// write section data
					std::cout << "Patching section["
					       	  << std::dec << std::setw(2)
						  << i << "] "
						  << std::hex
						  << section_addr
						  << "  " 
						  << std::hex
						  << section_sz - left
						  << " bytes"
						  << std::endl <<  std::endl;
#					if ENABLE_PATCH
					p_sec->set_data(p_src, section_sz - left);
					b_patched = true;
#					endif 	// ENABLE_PATCH
					if (b_verbose) {
						std::cout << "section_addr      "
							  << std::hex
				  			  << std::setw(8)
						  	  << section_addr
						  	  << std::endl;
						std::cout << "section_sz        "
							  << std::hex
				  			  << std::setw(8)
							  << section_sz
							  << std::endl;
						std::cout << "   left           "
							  << std::hex
				  			  << std::setw(8)
							  << left
							  << std::endl;
					}
					if (!left)
						break;	// section complete
				}
			} // end for
		}
	} // end for

	return b_patched;
}

// Utility code
// Return the machine name
const char *machine_name(Elf_Half e_machine)
{
	const char *name = machine_table[EM_NONE].str;

	for (int i = 0; i < sizeof(machine_table)/sizeof(struct machine_table_t); i++) {
		if (machine_table[i].key == e_machine) {
			name = machine_table[i].str;
			break;
		}
	} // end for

	return name;
}

// Return the ET type name
const char *type_name(Elf_Half e_type)
{
	const char *name = machine_table[ET_NONE].str;

	for (int i = 0; i < sizeof(type_table)/sizeof(struct type_table_t); i++) {
		if (type_table[i].key == e_type) {
			name = type_table[i].str;
			break;
		}
	} // end for

	return name;
}

// strips the path and path separators from the file name
char *strip_path(char *path)
{
	char *p_exec = NULL;

	if (path)
	{
	   p_exec = strrchr(path, '/');
	   if (p_exec == NULL)
		   p_exec = strrchr(path, '\\');
	   if (p_exec)
		   p_exec += 1;
	}

	return (p_exec) ? p_exec : path;
}

