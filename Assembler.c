////////////// DECLERATIONS /////////////

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE_LEN 500
#define MAX_LABEL_LEN 50
#define MAX_IMM_LEN 50
#define MAX_LABEL_NUM 512
#define MAX_INSTRACTION_NAME_LEN 4
#define MAX_REGISTER_NAME_LEN 4
#define MAX_INSTRACTIONS_NUM 512
#define WORD_INST_NAME "word"
#define HEX_LINE_LEN 9


typedef struct instruction {
	int opcode;
	int rd;
	int rs;
	int rt;
	int imm;

	int is_word_instruction;
	int data;
	int address;
} Instruction;


void check_dynamic_allocation(void* p);
int is_word_inst(char* line);
int get_label(char* line, char* label);
void save_labels(FILE* fp, char** labels);
int is_inst(char* line);
int label2address(char* token, char** labels);
void save_token(char* token, int tokens_count, Instruction* inst_pt, char** labels);
void parse_asm(char* curr_chr, char** labels, Instruction* inst_pt);
int inst_name2opcode(char* ints_name);
int reg_name_to_number(char* reg_name);
void update_mem_file(FILE* fp, Instruction* inst, int last_address);


////////////// Functions' Definitions /////////////


void check_file(FILE* fp) {
	if (fp == NULL) {
		perror("Error opnening the file");
		exit(1);
	}
}


void check_dynamic_allocation(void* p) {
	if (p == NULL) {
		fprintf(stderr,"Error opnening the file");
		exit(1);
	}
}


int is_word_inst(char* line) {
	// Gets a line from the assembly file.
	// Returns 1 if there's a ".word" instruction 
	// and 0 otherwise.
	int i = 0;
	char line_temp[MAX_LINE_LEN] = { 0 };
	while (line[i] != '\0') {
		line_temp[i] = tolower(line[i]);
		i++;
	}
	line_temp[i] = '\0';
	char* word_idx = strstr(line_temp, ".word");
	char* comment_idx = strstr(line_temp, "#");
	if (word_idx == NULL)  // zero occurences of ".word"
		return 0;
	if (comment_idx == NULL && word_idx != NULL)  // there's ".word" occuence and no comment in the  line
		return 1;
	if (comment_idx > word_idx)  // the occurence of ".word" is before the comment
		return 1;
	return 0;
}


int get_label(char* line, char* label) {
	// Assign the label from the line to the label argument.
	// If there's no label, the argument isn't changed.

	char token[MAX_LABEL_LEN + 1] = { 0 };
	int i = 0;
	int label_idx = 0;
	while (*line != '\n') {	// iterate until the end of the given line 
		if (*line == '#') {
			return 0;
		}
		else if (isalnum(*line)) {
			token[i] = tolower(*line);	// saving the label in lowercase
			i++;
		}
		else if (*line == ':') {	// if there's ':' before '#' then there's a label
			token[i] = '\0';
			strcpy(label, token);
			return 1;
		}
		line++;
	}
	return 0;
}


void save_labels(FILE* fp, char** labels) {
	// Gets a file pointer and a pointer to the labels' array.
	// Iterates the file, extracts the label (see get_label func)
	// and increment the address var only if there's an instruction
	// which isn't ".word" instruction.
	// Assumption: there's no label in the same line as .word command (see pdf for explanation).
	
	char* line = (char*)calloc(MAX_LINE_LEN+1, sizeof(char));
	check_dynamic_allocation(line);

	char* label = NULL;
	int address = 0;
	int is_label;
	
	while (fgets(line, MAX_LINE_LEN+1, fp) != NULL) {
		if (is_word_inst(line)==1) // if there's a .word instruction, the address isn't incremented
		{
			continue;
		}
		if (label == NULL) { 
			label = (char*)calloc(MAX_LABEL_LEN + 1, sizeof(char));
			check_dynamic_allocation(label);
			is_label = get_label(line, label);
		}
		if (is_label == 0) { 
			if (is_inst(line)==1) // case when instruction and no label
			{
				address++;
			}
		}
		else  // case when line has a label
		{ 
			if (is_inst(line) == 1) {  // case when instruction and no label
				strcpy(labels[address], label);
				address++;
			} 
			else {  // case when no label and no instruction
				continue;
			}
		}
		free(label);
		label = NULL;
	}
	free(line);
}


int is_inst(char* line) {
	// Gets a line and returns 1 if the line 
	// contains an instruction (including .word).
	// 0 otherwise.
	if (*line == '\n') // empty line
		return 0;

	while (*line != '\0') {	
		
		if (*line == '#')   // line consist a comment only
			return 0;
		
		if (isalpha(*line)) {  // if there's a letter: continue iterate the line
			line++;
			
			while (*line != '\0') {
				
				if (*line == '#') { 
					// line contains an instruction and a comment 
					return 1;
				}
				if (*line == ':') {
					line++;
					
					if (*line=='\n') // line contain a label only
						return 0;

					while (*line != '\0')
					{
						if (*line == '#') // line contains a label and a comment
							return 0;

						if (isalpha(*line)) { // line contains a label and an instruction
							return 1;
						}
						line++;
					}
				}
				if (*line=='\n') // line conteins only an instruction
					return 1;
				line++;
			}
		}
		line++;
	}
	return 0;  // line conteins only spaces
}


int label2address(char* token, char** labels) {
	// Gets part of a line (token) and a pointer 
	// to the labels array.
	// Returns the address that the label is pointing to.
	int i = 0;
	while (i < MAX_LABEL_NUM) { 
		if (labels[i] != 0) {
			if (strcmp(labels[i], token)==0)
				return i;
		}
		i++;
	}
	perror("label not found");
	exit(1);
}


void save_token(char* token, int tokens_count, Instruction* inst_pt, char** labels) {
	/* Gets a part of an instruction (token) and the location in the
	   instruction (tokens_count).
	   Saves the part in the suitable variable in the Instruction struct.
	   Updates the is_word_instruction field in the instruction struct.
	   The .word instruction is saved in the same instruction struct
	   (see the struct definition). */
	char* endpt;  // needed for the strtoll func. Not used.
	switch (tokens_count) {
		case 1: {	// opcode
			if (is_word_inst(token) == 1)
				inst_pt->is_word_instruction = 1;
			else {
				inst_pt->is_word_instruction = 0;
				inst_pt->opcode = inst_name2opcode(token);
			}
			break;
		}
		case 2: { // rd or address
			if (inst_pt->is_word_instruction == 1) {
				inst_pt->address = (int) (strtoll(token, &endpt, 0));
			}
			else
				inst_pt->rd = reg_name_to_number(token);
			break;
		}
		case 3: { // rs or data
			if (inst_pt->is_word_instruction == 1) {
				inst_pt->data = (int) strtoll(token, &endpt, 0);
			}
			else
				inst_pt->rs = reg_name_to_number(token);
			break;
		}
		case 4: // rt
			inst_pt->rt = reg_name_to_number(token);
			break;
		case 5: { // imm 
			// if contains a label: convert to the address in hex
			// if contains num: just convert to hex
			if (isalpha(*token))
				inst_pt->imm = label2address(token, labels);
			else
				inst_pt->imm = (int) strtoll(token, &endpt, 0);
			break;
		}
	}
}

void parse_asm(char* curr_chr, char** labels, Instruction* inst_pt) {
	// Split the given line (curr_chr) into its different parts 
	// and send each part to the suitable saving function.
	// Converts all letters to lower case.
	int tokens_count = 0;
	int token_letter = 0;	// Index for writing to the token
	char token[MAX_IMM_LEN + 1] = { 0 };	// Set to the imm len because its the longest field
	char* next_chr = curr_chr+1;
	
	while (*curr_chr != '\0') {	// iterate until the end of the given line 
		if (*curr_chr == '#') // line start with a comment 
			return;
		else if (isalnum(*curr_chr) || *curr_chr == '-' || *curr_chr == '.') {
			// if letter, number, minus sign or '.' (for .word instructions) then save
			token[token_letter] = tolower(*curr_chr);
			token_letter++;
			if (*next_chr == ',' || isspace(*next_chr) || *next_chr == '#') {
				// if next char is ',' or space-type or '#' then end of token
				tokens_count++;
				token[token_letter] = '\0';
				save_token(token, tokens_count, inst_pt, labels);
				token_letter = 0;	// reset index
			}
		}
		else if (*curr_chr == ':') {
			// if there's ':' before '#' then there's a label and
			// the token's parameters are cleared
			tokens_count = 0;
			token_letter = 0;
		}
		curr_chr++;
		next_chr++;
	}

}


int inst_name2opcode(char* ints_name) {
	// Returns the instruction's number.
	// Raise an error if the name doesn't exist.
	if (strcmp(ints_name, "add") == 0) return 0;
	if (strcmp(ints_name, "sub") == 0) return 1;
	if (strcmp(ints_name, "and") == 0) return 2;
	if (strcmp(ints_name, "or") == 0) return 3;
	if (strcmp(ints_name, "sll") == 0) return 4;
	if (strcmp(ints_name, "sra") == 0) return 5;
	if (strcmp(ints_name, "srl") == 0) return 6;
	if (strcmp(ints_name, "beq") == 0) return 7;
	if (strcmp(ints_name, "bne") == 0) return 8;
	if (strcmp(ints_name, "blt") == 0) return 9;
	if (strcmp(ints_name, "bgt") == 0) return 10;
	if (strcmp(ints_name, "ble") == 0) return 11;
	if (strcmp(ints_name, "bge") == 0) return 12;
	if (strcmp(ints_name, "jal") == 0) return 13;
	if (strcmp(ints_name, "lw") == 0) return 14;
	if (strcmp(ints_name, "sw") == 0) return 15;
	if (strcmp(ints_name, "halt") == 0) return 19;
	else {
		perror("Invalid opcode");
		exit(1);
	}
}

int reg_name_to_number(char* reg_name) {
	// Returns the register's number.
	// Raise an error if the name doesn't exist.

	if (strcmp(reg_name, "zero") == 0) return 0;
	if (strcmp(reg_name, "imm") == 0) return 1;
	if (strcmp(reg_name, "v0") == 0) return 2;
	if (strcmp(reg_name, "a0") == 0) return 3;
	if (strcmp(reg_name, "a1") == 0) return 4;
	if (strcmp(reg_name, "t0") == 0) return 5;
	if (strcmp(reg_name, "t1") == 0) return 6;
	if (strcmp(reg_name, "t2") == 0) return 7;
	if (strcmp(reg_name, "t3") == 0) return 8;
	if (strcmp(reg_name, "s0") == 0) return 9;
	if (strcmp(reg_name, "s1") == 0) return 10;
	if (strcmp(reg_name, "s2") == 0) return 11;
	if (strcmp(reg_name, "gp") == 0) return 12;
	if (strcmp(reg_name, "sp") == 0) return 13;
	if (strcmp(reg_name, "fp") == 0) return 14;
	if (strcmp(reg_name, "ra") == 0) return 15;
	else {
		perror("Invalid register");
		exit(1);
	}
}

void update_mem_file(FILE* fp, Instruction* inst, int curr_address) {
	// Write the instructions in hex, to the memin file
	// If .word instruction: if the address to be rewriten 
	// is smaller that the .word's address, this line is changed.
	// Otherwise, the lines untill this address filled with 0's.  
	if (inst->is_word_instruction == 1) {
		if (inst->address <= curr_address) {
			fseek(fp, inst->address * HEX_LINE_LEN, SEEK_SET);
			fprintf(fp, "%08X", inst->data);
			return;
		}
		else {
			for (int i = curr_address; i < inst->address; i++) {
				fprintf(fp, "%08X\n", 0);
			}
			fprintf(fp, "%08X", inst->data);
			return;
		}
	}
	if (inst->imm < 0)
		inst->imm &= 0X00000FFF; // the hex representation without the 5 first digits (F)
	fseek(fp, curr_address * HEX_LINE_LEN,SEEK_SET);
	fprintf(fp, "%02X%01X%01X%01X%03X\n", inst->opcode, inst->rd, inst->rs, inst->rt, inst->imm);
}


void free_labels(char** labels) {
	for (int i = 0; i < MAX_LABEL_NUM; i++) {
		free(labels[i]);
	}
}


int main(int argc, char* argv[]) {
	// (1) Saves the labels to a dynamic allocated memory
	// (2) Saves a line from the asm file, calls the parsing func and the writing func
	// (3) again until EndOfFile

	char* asm_prog_path = argv[1];
	char* mem_path = argv[2];

	FILE* asm_fp = fopen(asm_prog_path, "r");
	check_file(asm_fp);
	
	FILE* mem_fp = fopen(mem_path, "w");
	check_file(mem_fp);

	// allocate memory for labels
	char** labels = (char**) calloc(MAX_LABEL_NUM, sizeof(char*));
	check_dynamic_allocation(labels);
	for (int i = 0; i < MAX_LABEL_NUM; i++) {
		labels[i] = (char*)calloc(MAX_LABEL_LEN+1, sizeof(char));
		check_dynamic_allocation(labels[i]);
	}

	save_labels(asm_fp, labels);
	fseek(asm_fp, 0, SEEK_SET);  // Return the file pointer to the beginning of the file

	// Allocate memory for an instruction
	Instruction* inst_pt = (Instruction*)calloc(1, sizeof(Instruction));
	check_dynamic_allocation(inst_pt);

	// Parse and save line-by-line
	int	curr_address = 0;
	char line[MAX_LINE_LEN + 1] = { 0 };
	while (fgets(line, MAX_LINE_LEN + 1, asm_fp) != NULL) {
		parse_asm(line, labels, inst_pt);
		if (is_inst(line) == 1) {
			update_mem_file(mem_fp, inst_pt, curr_address);
			if (inst_pt->is_word_instruction == 0)
				curr_address++;
		}
	}

	free_labels(labels);
	fclose(mem_fp);
	fclose(asm_fp);

}
