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

#ifndef _MEMLIM_H
#define _MEMLIM_H

#define MAX_MEM 300000

#endif

#ifndef    _DATASET_H
#define    _DATASET_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#ifdef HAVE_BOOST
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#endif
#include <fstream>

using namespace std;

// map of words/terms [string => int]
typedef map<string, int> mapword2id;
// map of words/terms [int => string]
typedef map<int, string> mapid2word;

class document 
{
public:
#ifdef HAVE_BOOST
	friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int versin){
       ar & words;
       ar & rawstr;
       ar & length; 
    }
#endif
	vector<int> words;
#if 0
	int * words;
#endif

    string rawstr;
	int length   ; // the length of words in one document
    
    document() : words() {
		//words = mullptr;
		rawstr = "";
		length = 0;
	}

    explicit document(int length) : words(length,0) {
		this->length = length;
		rawstr = "";
		//words = new int[length];	
	}

    document(int length, vector<int> words) {
		this->length = length;
		rawstr = "";
		this->words = words;
	}

    document(int length, vector<int> words, const string &rawstr) {
		this->length = length;
		this->rawstr = rawstr;
		this->words = words; 
	}

	explicit document(vector<int> &doc) {
		this->length = doc.size();
		rawstr = "";
		this->words = doc;
	}

	document(vector<int> &doc, const string &rawstr) {
		this->length = doc.size();
		this->rawstr = rawstr;
		this->words = doc;
	}

	~document() {
	}
};

class dataset 
{
public:
	vector<string> file_names;
    vector<int> start_index;
    vector<document> docs;
    document ** _docs; // used only for inference
	map<int, int> _id2id;   // also used only for inference,  _id:increment word index for unseen dataset,  id:increment word index in train dataset.
    int M; // number of documents
    int V; // number of words
    int cur_size;
    int cur_index;//index the position in file_names
    string dir;

    dataset() : docs(), file_names(), start_index(), dir() {
		//docs = nullptr;
		_docs = nullptr;
		M = 0;
		V = 0;
        cur_size = 0;
        cur_index = 0;
	}

    explicit dataset(int M) : docs(), file_names(), start_index(), dir() {
		this->M = M;
		this->V = 0;
		//docs = new document*[M];	
        cur_size = 0;
        cur_index = 0;
		_docs = nullptr;
	}

    ~dataset() 
	{
	if (_docs) 
	{
	    for (int i = 0; i < M; i++) 
		{
		delete _docs[i];		
	    }
	}
	delete[] _docs;	
    }
    
    void deallocate() 
	{
		docs.clear();

	if (_docs) 
	{
	    for (int i = 0; i < M; i++) 
		{
		delete _docs[i];
	    }
	}
	delete[] _docs;
		_docs = nullptr;
    }

	void clear()
	{
		deallocate();
	}

#ifdef HAVE_BOOST
	bool write_to_disk(int index){
      ofstream ofs;//(file_names[index]);//(file_names.back()) ;
      ofs.open(file_names[index], ofstream::out|ofstream::trunc);
      boost::archive::text_oarchive oa(ofs);
      oa << docs; 
      ofs.close();
      return true;
    }
     
    bool read_from_disk(int index){
       /*
        *for the data part no need to write before read new, bc data never changes
       if(cur_index < start_index.size()){
         //the last write increase the cur_index out of range
         write_to_disk(cur_index);
       }*/
       ifstream ifs(file_names[index]);
       boost::archive::text_iarchive ia(ifs);
       docs.clear();
       ia >> docs;
       cur_size = docs.size();
       cur_index = index;
       return true;
    }
#endif

    document& get_docs(int idx) {
       //required doc in memory
       if(!(cur_index < start_index.size() && idx >= start_index[cur_index] &&
          (cur_index==start_index.size()-1||idx<start_index[cur_index+1]))){
#ifdef HAVE_BOOST
		  int start = 0;
          int end = start_index.size()-1; 
          while(start + 1 < end){
            int mid = start + ((end-start)>>1);
            if(idx < start_index[mid]){
              end = mid;
            }else{
              start = mid;
            }
          }
          int dex = (idx>=start_index[end])?end:start;
          read_from_disk(dex); 
#else
		   throw new exception("GibbsLDA: partial loading of documents is only supported when Boost is built-in.");
#endif
	   }   
           
       return docs[idx-start_index[cur_index]];
    }

	//doc->words index for training dataset
    void add_doc(document &doc, int idx) {
		if (0 <= idx && idx < M) {
            if(docs.empty()){
              start_index.push_back(idx);
            }
		    docs.push_back(doc);
            cur_size++;
            //store into hard disk if full  
            if(docs.size() > MAX_MEM/4){
#ifdef HAVE_BOOST
			  file_names.push_back(dir+"data_"+std::to_string(cur_index)+".data");
              write_to_disk(cur_index); 
              docs.clear();
              cur_size = 0;
              cur_index++; 
#else
				throw new exception("GibbsLDA: docs array running out of bounds. Partial storing of documents is only supported when Boost is built-in.");
#endif
			}      
		}
	}

	//_doc->words index for unseen dataset
    void _add_doc(document * doc, int idx) 
	{
	if (0 <= idx && idx < M) 
	{
	    _docs[idx] = doc;
	}
    }       

	static int write_wordmap(const string &wordmapfile, mapword2id *pword2id);

	static int read_wordmap(const string &wordmapfile, mapword2id *pword2id);

	static int read_wordmap(const string &wordmapfile, mapid2word *pid2word);

	int read_trndata(const string &dfile, const string &wordmapfile);

	int read_newdata(const string &dfile, const string &wordmapfile);

	int read_newdata_withrawstrs(const string &dfile, const string &wordmapfile);
};

#endif

