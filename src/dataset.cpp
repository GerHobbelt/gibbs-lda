/*
 * Copyright (C) 2007 by
 * 
 * 	Xuan-Hieu Phan
 *	hieuxuan@ecei.tohoku.ac.jp or pxhieu@gmail.com
 * 	Graduate School of Information Sciences
 * 	Tohoku University
 *
 * GibbsLDA++ is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * GibbsLDA++ is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GibbsLDA++; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <cstdio>
#include <cstdlib>
#include "constants.h"
#include "strtokenizer.h"
#include "dataset.h"

using namespace std;

/**
 * Not that these functions are used for serializing and deserializing. Do not write debug information in the write 
 * function if you are also not prepared to adapt the read function.
 */
int dataset::write_wordmap(const string &wordmapfile, mapword2id *pword2id) {
	FILE *fout = fopen(wordmapfile.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to write!\n", wordmapfile.c_str());
		return 1;
	}

	mapword2id::iterator it;
	fprintf(fout, "%zu\n", pword2id->size());
	for (it = pword2id->begin(); it != pword2id->end(); it++) {
		fprintf(fout, "%s %d\n", (it->first).c_str(), it->second);
	}

	fclose(fout);

	return 0;
}

int dataset::read_wordmap(const string &wordmapfile, mapword2id *pword2id) {
	pword2id->clear();

	FILE *fin = fopen(wordmapfile.c_str(), "r");
	if (!fin) {
		printf("Cannot open file %s to read!\n", wordmapfile.c_str());
		return 1;
	}

	char buff[BUFF_SIZE_SHORT];
	string line;

	char *endptr = nullptr;
	fgets(buff, BUFF_SIZE_SHORT - 1, fin);
	int nwords = (int) strtol(buff, &endptr, 10);
	//int nwords = atoi(buff);

	for (int i = 0; i < nwords; i++) {
		fgets(buff, BUFF_SIZE_SHORT - 1, fin);
		line = buff;

		strtokenizer strtok(line, " \t\r\n");
		if (strtok.count_tokens() != 2) {
			continue;
		}

		//pword2id->insert(pair<string, int>(strtok.token(0), atoi(strtok.token(1).c_str())));
		pword2id->insert(pair<string, int>(strtok.token(0), (int) strtol(strtok.token(1).c_str(), &endptr, 10)));
	}

	fclose(fin);

	return 0;
}

int dataset::read_wordmap(const string &wordmapfile, mapid2word *pid2word) {
	pid2word->clear();

	FILE *fin = fopen(wordmapfile.c_str(), "r");
	if (!fin) {
		printf("Cannot open file %s to read!\n", wordmapfile.c_str());
		return 1;
	} else {
		printf("Opened file %s to read wordmap\n", wordmapfile.c_str());
	}

	char buff[BUFF_SIZE_SHORT];
	string line;

	char *endptr = nullptr;
	fgets(buff, BUFF_SIZE_SHORT - 1, fin);
	int nwords = (int) strtol(buff, &endptr, 10);

	for (int i = 0; i < nwords; i++) {
		fgets(buff, BUFF_SIZE_SHORT - 1, fin);
		line = buff;

		strtokenizer strtok(line, " \t\r\n");
		if (strtok.count_tokens() != 2) {
			continue;
		}

		pid2word->insert(pair<int, string>((int) strtol(strtok.token(1).c_str(), &endptr, 10), strtok.token(0)));
	}

	fclose(fin);

	return 0;
}

int dataset::read_trndata(const string &dfile, const string &wordmapfile) {
	mapword2id word2id;

	FILE *fin = fopen(dfile.c_str(), "r");
	if (!fin) {
		printf("Cannot open file %s to read!\n", dfile.c_str());
		return 1;
	}

	mapword2id::iterator it;
	char buff[BUFF_SIZE_LONG];
	string line;

	char *endptr = nullptr;
	// get the number of documents
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	M = (int) strtol(buff, &endptr, 10);
	if (M <= 0) {
		printf("No document available!\n");
		return 1;
	}

	// allocate memory for corpus
	if (docs) {
		deallocate();
	} else {
		docs = new document *[M];
	}

	// set number of words to zero
	V = 0;

	for (int i = 0; i < M; i++) {
		// read next line from file
		fgets(buff, BUFF_SIZE_LONG - 1, fin);
		line = buff;
		strtokenizer strtok(line, " \t\r\n");

		// length is number of non-unique words
		int length = strtok.count_tokens();

		if (length <= 0) {
			printf("Invalid (empty) document!\n");
			deallocate();
			M = V = 0;
			return 1;
		}

		// allocate new document
		auto *pdoc = new document(length);

		// iterate over all words found
		for (int j = 0; j < length; j++) {
			it = word2id.find(strtok.token(j));
			if (it == word2id.end()) {
				// word not found, i.e., new word, add to vocabulary, set its id to vocabulary size
				pdoc->words[j] = word2id.size();
				word2id.insert(pair<string, int>(strtok.token(j), word2id.size()));
			} else {
				// existing word, set its id in map
				pdoc->words[j] = it->second;
			}
		}

		// add new doc to the corpus
		add_doc(pdoc, i);
	}

	fclose(fin);

	// write word map to file
	if (write_wordmap(wordmapfile, &word2id)) {
		return 1;
	}

	// update number of words
	V = word2id.size();

	return 0;
}

int dataset::read_newdata(const string &dfile, const string &wordmapfile) {
	mapword2id word2id;
	map<int, int> id2_id;

	read_wordmap(wordmapfile, &word2id);
	if (word2id.empty()) {
		printf("No word map available!\n");
		return 1;
	}

	FILE *fin = fopen(dfile.c_str(), "r");
	if (!fin) {
		printf("Cannot open file %s to read!\n", dfile.c_str());
		return 1;
	}

	mapword2id::iterator it;
	map<int, int>::iterator _it;
	char buff[BUFF_SIZE_LONG];
	string line;

	char *endptr = nullptr;
	// get number of new documents
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	M = (int) strtol(buff, &endptr, 10);
	if (M <= 0) {
		printf("No document available!\n");
		return 1;
	}

	// allocate memory for corpus
	if (docs) {
		deallocate();
	} else {
		docs = new document *[M];
	}
	_docs = new document *[M];

	// set number of words to zero
	V = 0;

	for (int i = 0; i < M; i++) {
		fgets(buff, BUFF_SIZE_LONG - 1, fin);
		line = buff;
		strtokenizer strtok(line, " \t\r\n");
		int length = strtok.count_tokens();

		vector<int> doc;
		vector<int> _doc;
		for (int j = 0; j < length; j++) {
			it = word2id.find(strtok.token(j));
			if (it == word2id.end()) {
				// word not found, i.e., word unseen in training data
				// do anything? (future decision)
			} else {
				int _id;
				_it = id2_id.find(it->second);
				if (_it == id2_id.end()) {
					_id = id2_id.size();
					id2_id.insert(pair<int, int>(it->second, _id));
					_id2id.insert(pair<int, int>(_id, it->second));
				} else {
					_id = _it->second;
				}

				doc.push_back(it->second);
				_doc.push_back(_id);
			}
		}

		// allocate memory for new doc
		auto *pdoc = new document(doc);
		auto *_pdoc = new document(_doc);

		// add new doc
		add_doc(pdoc, i);
		_add_doc(_pdoc, i);
	}

	fclose(fin);

	// update number of new words
	V = id2_id.size();

	return 0;
}

int dataset::read_newdata_withrawstrs(const string &dfile, const string &wordmapfile) {
	mapword2id word2id;
	map<int, int> id2_id;

	read_wordmap(wordmapfile, &word2id);
	if (word2id.empty()) {
		printf("No word map available!\n");
		return 1;
	}

	FILE *fin = fopen(dfile.c_str(), "r");
	if (!fin) {
		printf("Cannot open file %s to read!\n", dfile.c_str());
		return 1;
	}

	mapword2id::iterator it;
	map<int, int>::iterator _it;
	char buff[BUFF_SIZE_LONG];
	string line;

	char *endptr = nullptr;
	// get number of new documents
	fgets(buff, BUFF_SIZE_LONG - 1, fin);
	M = (int) strtol(buff, &endptr, 10);
	if (M <= 0) {
		printf("No document available!\n");
		return 1;
	}

	// allocate memory for corpus
	if (docs) {
		deallocate();
	} else {
		docs = new document *[M];
	}
	_docs = new document *[M];

	// set number of words to zero
	V = 0;

	for (int i = 0; i < M; i++) {
		fgets(buff, BUFF_SIZE_LONG - 1, fin);
		line = buff;
		strtokenizer strtok(line, " \t\r\n");
		int length = strtok.count_tokens();

		vector<int> doc;
		vector<int> _doc;
		for (int j = 0; j < length - 1; j++) {
			it = word2id.find(strtok.token(j));
			if (it == word2id.end()) {
				// word not found, i.e., word unseen in training data
				// do anything? (future decision)
			} else {
				int _id;
				_it = id2_id.find(it->second);
				if (_it == id2_id.end()) {
					_id = id2_id.size();
					id2_id.insert(pair<int, int>(it->second, _id));
					_id2id.insert(pair<int, int>(_id, it->second));
				} else {
					_id = _it->second;
				}

				doc.push_back(it->second);
				_doc.push_back(_id);
			}
		}

		// allocate memory for new doc
		auto *pdoc = new document(doc, line);
		auto *_pdoc = new document(_doc, line);

		// add new doc
		add_doc(pdoc, i);
		_add_doc(_pdoc, i);
	}

	fclose(fin);

	// update number of new words
	V = id2_id.size();

	return 0;
}

