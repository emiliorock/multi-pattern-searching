/*
	Written By Mengxin Huang
	z5013846
	Algorithm used: Boyer-Moore and Wu-Manber
	Reference to: http://webglimpse.net/pubs/TR94-17.pdf
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <string.h>
#include <vector>  
#include <algorithm>

using namespace std;

struct compare {
	bool operator ()(pair<string, int> p1, pair<string, int> p2) {
		//return (p1.second > p2.second);
		if (p1.second == p2.second) return (p1.first < p2.first);
		else return (p1.second > p2.second);
	}
};

char *target_dir;
char search_terms[5][256];

// last occ array
int last[5][127];
// wu-manber hash table
int hash_table[256][256];
int frequency[5] = {0};

int num_of_terms = 0;
int count_term = 0;
// the minimum length of the search terms
int min_len = 256;
// group every 2 characters together
int group_size = 2;
// the default #of position we want to move
int offset = 0;
int t_pos = 0;
int p_pos = 0;
int not_match_flag = 0;
int end_pos = 0;

ifstream file;
// the order of the search terms in command line arguments
vector<pair<int, string> >index_pattern;
vector<pair<int, string> >::iterator sit;
// finally sort the files
vector<pair<string, int> >file_set;
vector<pair<string, int> >::iterator fit;

void BM_search();
void WM_search();
void last_occurrence(int, char *);
void boyer_moore(int, int, char *, int, char *);
void wu_manber(int, string);
void sort_files();
void initialize_array();
bool is_same(int, int);

int main(int argc, char* argv[]) {

	target_dir = argv[1];
	strcat(target_dir, "/");

	// "-s" is specified
	if (strcmp(argv[3], "-s") == 0) {		
		int i = 5;
		while (i < argc) {
			strcpy(search_terms[i - 5], argv[i]);
			num_of_terms++;
			i++;
		}
	}

	// "-s" is not specified
	else {
		if (argc > 8) {
			cout << "too many arguments" << endl;
			return 1;
		}
		
		int i = 3;
		while (i < argc) {
			strcpy(search_terms[i - 3], argv[i]);
			num_of_terms++;
			i++;
		}
	}
	
	// the minimum length of the search terms
	// if min_len = 1, use BM, else, use WM
	for (int i = 0;i < num_of_terms;i++) {
		if (strlen(search_terms[i]) < min_len) {
			min_len = strlen(search_terms[i]);
		}
	}

	if (min_len > 1) {
		// group every 2 characters together until offset
		// use the 2 ASCII values as the key of an 2-D array
		offset = min_len - group_size + 1;
		for (int i = 0; i < 256;i++) {
			for (int j = 0;j < 256;j++) {
				hash_table[i][j] = offset;
			}
		}
		// convert search terms to lower case
		// and compute hash table
		for (int i = 0;i< num_of_terms;i++) {
			for (int j = 0;j < strlen(search_terms[i]);j++) {
				//search_terms[i][j] = tolower(search_terms[i][j]);
				if (search_terms[i][j] >= 'A' && search_terms[i][j] <= 'Z') {
					search_terms[i][j] += 32;
				}
				if (j > 0 && j <= offset) {
					// case insensitive
					hash_table[search_terms[i][j - 1]][search_terms[i][j]] = offset - j;
					hash_table[search_terms[i][j - 1] - 32][search_terms[i][j]] = offset - j;
					hash_table[search_terms[i][j - 1]][search_terms[i][j] - 32] = offset - j;
					hash_table[search_terms[i][j - 1] - 32][search_terms[i][j] - 32] = offset - j;
					if (offset - j == 0) {
						index_pattern.push_back(make_pair(i, search_terms[i]));
					}
				}
			}
		}

		// apply wu-manber search
		WM_search();
	}
	
	else {
		// initialize array
		initialize_array();
		// convert search terms to lower case, and compute last occurrence
		for (int i = 0;i< num_of_terms;i++) {
			for (int j = 0;j < strlen(search_terms[i]);j++) {
				search_terms[i][j] = tolower(search_terms[i][j]);
				last[i][search_terms[i][j]] = j;
			}
		}
		// apply boyer-moore search
		BM_search();
	}

	return 0;
}

void WM_search() {
	DIR *dir;
	struct dirent *dirEntry;
	if (dir = opendir(target_dir)) {
  		while (dirEntry = readdir(dir)) {
  			if(strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0) { 				
  				char filename[100];
  				strcpy(filename, target_dir);
	  			strcat(filename, dirEntry->d_name);
	  			file.open(filename);
	  			// initialize
	  			for (int k = 0;k < num_of_terms;k++) {
	  				frequency[k] = 0;
	  			}
	  			while (!file.eof()) {
	  				string line;
	  				getline(file, line);
	  				wu_manber(offset, line);
	  			}
	  			file.close();
	  			int flag = 0;
	  			int sum_freq = 0;
	  			// check if every pattern has appeared in the file
	  			for (int k = 0;k < num_of_terms;k++) {
	  				if (frequency[k] == 0) {
	  					flag = 1;
	  					break;
	  				}
	  				sum_freq += frequency[k];
	  			}
	  			if (flag == 0) {
	  				file_set.push_back(make_pair(dirEntry->d_name, sum_freq));
	  			}
  			}
  		}

  	}
	sort_files();
  	closedir(dir);
}

// check if 2 char are the same, case insensitive
bool is_same(int c1, int c2) {
	if (c1 == c2) return true;
	else if (c1 == c2 + 32) return true;
	else if (c1 == c2 - 32) return true;
	else return false;
}

void wu_manber(int i, string text) {
	while (text.size() && i < text.size() && min_len <= text.size()) {
		if ((int)text[i-1] < 0 || (int)text[i-1] >= 256 || (int)text[i] < 0 || (int)text[i] >= 256) {
			i += offset;
		}
		else if (hash_table[text[i-1]][text[i]] == 0) {
			for (sit = index_pattern.begin();sit < index_pattern.end();sit++) {
				// compare prefix, if match, compare whole pattern
				// if not, shrift
				if (is_same(text[i - offset], sit->second[0]) && is_same(text[i - offset + 1], sit->second[1])) {
					t_pos = i - offset;
					p_pos = 0;
					not_match_flag = 0;
					end_pos = sit->second.size();
					while (p_pos < end_pos) {
						if (!is_same(text[t_pos], sit->second[p_pos])) {
							not_match_flag = 1;
							break;
						}
						t_pos++;
						p_pos++;
					}

					// if pattern matches
					if (not_match_flag == 0) {
						frequency[sit->first]++;
					}
				}	
			}
			i += min_len;
		} 
		else {
			i += hash_table[text[i-1]][text[i]];
		}
	}
}

void sort_files() {
	sort(file_set.begin(), file_set.end(), compare());
	for (fit = file_set.begin();fit < file_set.end();fit++) {
	  	cout << fit->first << endl;
	}
}

void initialize_array () {
	for (int i = 0;i < num_of_terms;i++) {
		for (int j = 0;j < 127;j++) {
			last[i][j] = -1;
		}
	}
}

void last_occurrence(int n, char *str) {
	for (int i = 0;i < strlen(str);i++) {
		last[n][str[i]] = i;
	}
}

void BM_search() {
	DIR *dir;
	struct dirent *dirEntry;
	if (dir = opendir(target_dir)) {
  		while (dirEntry = readdir(dir)) {
  			if(strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0) {
  				int flag = 0;
  				int sum_term = 0;
  				for (int i = 0;i < num_of_terms;i++) {
  					char filename[100];
  					strcpy(filename, target_dir);
	  				strcat(filename, dirEntry->d_name);
	  				file.open(filename);
	  				count_term = 0;
	  				while (!file.eof()) {
	  					string line;
	  					getline(file, line);
	  					char *text = (char *)line.c_str();
	  					for (int k = 0;k < strlen(text);k++) {
	  						text[k] = tolower(text[k]);
	  					}
  						if (strlen(text) >= strlen(search_terms[i])) {
  							boyer_moore(i, strlen(search_terms[i]) - 1, text, strlen(search_terms[i]) - 1, search_terms[i]);
  						}
  					}
  					file.close();
  					if (count_term == 0)  {
  						flag = 1;
  						break;
  					}
  					else {
  						sum_term += count_term;
  					}
  				}
  				// if all the search terms are in the file
  				if (flag == 0) {
  					file_set.push_back(make_pair(dirEntry->d_name, sum_term));
  				}
  			}
  		}
  		sort_files();
  		closedir(dir);
	} 
}

void boyer_moore(int n, int i, char *text, int j, char *pattern) {
	// if same, compare the former letter
	if (text[i] == pattern[j]) {
		if ((j == 0) && (i < strlen(text) - strlen(pattern))) {
			count_term++;
			j = strlen(pattern) - 1;
			i += strlen(pattern);
			boyer_moore(n, i, text, j, pattern);
		}
		else if ((j == 0) && (i == strlen(text) - strlen(pattern))) {
			count_term++;
		}
		else {
			i--;
			j--;
			boyer_moore(n, i, text, j, pattern);
		}
	}
	// not same, shrift
	else {
		// the end of the line
 		if (i > strlen(text) - 1) {
		}
		else if (last[n][text[i]] != -1) {
			// case 1:
			if (last[n][text[i]] < last[n][pattern[j]] && j >= last[n][text[i]]) {
				j = strlen(pattern) - 1;
				i += j - last[n][text[i]];
				boyer_moore(n, i, text, j, pattern);
			}
			// case 2:
			else {
				i += strlen(pattern) - j;
				j = strlen(pattern) - 1;
				boyer_moore(n, i, text, j, pattern); 
			}
		}
		// case 3: shritf whole pattern to right 
		else {
			j = strlen(pattern) - 1;
			i += strlen(pattern);
			if (i < strlen(text)) {
				boyer_moore(n, i, text, j, pattern);
			}
		}
	}
}
