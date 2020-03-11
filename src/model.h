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

/* 
 * References:
 * + The Java code of Gregor Heinrich (gregor@arbylon.net)
 *   http://www.arbylon.net/projects/LdaGibbsSampler.java
 * + "Parameter estimation for text analysis" by Gregor Heinrich
 *   http://www.arbylon.net/publications/text-est.pdf
 */

#ifndef	_MODEL_H
#define	_MODEL_H

#include "constants.h"
#include "dataset.h"
#define MOD_THETA_SAVE 300000000

using namespace std;

// LDA model
class model {
public:
    // fixed options
    string wordmapfile;		// file that contains word map [string -> integer id]
    string trainlogfile;	// training log file
    string tassign_suffix;	// suffix for topic assignment file
    string theta_suffix;	// suffix for theta file
    string phi_suffix;		// suffix for phi file
    string others_suffix;	// suffix for file containing other parameters
    string twords_suffix;	// suffix for file containing words-per-topics

    string dir;			// model directory
    string dfile;		// data file    
    string model_name;		// model name
    int model_status;		// model status:
				// MODEL_STATUS_UNKNOWN: unknown status
				// MODEL_STATUS_EST: estimating from scratch
				// MODEL_STATUS_ESTC: continue to estimate the model from a previous one
				// MODEL_STATUS_INF: do inference

    dataset * ptrndata;	// pointer to training dataset object
    dataset * pnewdata; // pointer to new dataset object

    mapid2word id2word; // word map [int => string]
    
    // --- model parameters and variables ---    
    int M; // dataset size (i.e., number of docs)
    int V; // vocabulary size
    int K; // number of topics
    double alpha, beta; // LDA hyperparameters 
    int niters; // number of Gibbs sampling iterations
    int liter; // the iteration at which the model was saved
    int savestep; // saving period
    int twords; // print out top words per each topic
    int withrawstrs;

    double * p; // temp variable for sampling

    vector<vector<int> > z; 
    vector<string> z_file_names;
    vector<int> z_start_index;
    int z_cur_size;
    int z_cur_index;
    
    vector<int>& get_z(int idx){
      return get_docs(idx,z_file_names,z_start_index,z_cur_size,z_cur_index,z);
    }
    
    void add_z(vector<int>& e, int idx){
       add_element(e,idx,z_file_names,z_start_index,z_cur_size,z_cur_index,z,"Z"); 
    }
    
        
    //int ** z; // topic assignments for words, size M x doc.size()
    int ** nw; // cwt[i][j]: number of instances of word/term i assigned to topic j, size V x K
    vector<vector<int> > nd;
    vector<string> nd_file_names;
    vector<int> nd_start_index;
    int nd_cur_size;
    int nd_cur_index;
    
    vector<int>& get_nd(int idx){
      return get_docs(idx,nd_file_names,nd_start_index,nd_cur_size, nd_cur_index,nd);
    } 
    
    void add_nd(vector<int>&e, int idx){
       add_element(e,idx,nd_file_names,nd_start_index,nd_cur_size,nd_cur_index,nd,"ND"); 
    }

    //int ** nd; // na[i][j]: number of words in document i assigned to topic j, size M x K
    int * nwsum; // nwsum[j]: total number of words assigned to topic j, size K
    int * ndsum; // nasum[i]: total number of words in document i, size M
    vector<vector<double> > theta;
    vector<string> theta_file_names;
    vector<int> theta_start_index;
    int theta_cur_size;
    int theta_cur_index;
    vector<double>& get_theta(int idx){
      return get_docs(idx,theta_file_names,theta_start_index,theta_cur_size, theta_cur_index,theta);
    } 
    
    void add_theta(vector<double>&e, int idx){
       add_element(e,idx,theta_file_names,theta_start_index,theta_cur_size,theta_cur_index,theta,"THETA"); 
    }

    //double ** theta; // theta: document-topic distributions, size M x K
    double ** phi; // phi: topic-word distributions, size K x V
    
    // for inference only
    int inf_liter;
    int newM;
    int newV;
    int ** newz;
    int ** newnw;
    int ** newnd;
    int * newnwsum;
    int * newndsum;
    double ** newtheta;
    double ** newphi;
    // --------------------------------------
    
    model() {
	set_default_values();
    }
          
    ~model();
    
    // set default values for variables
    void set_default_values();   

    // parse command line to get options
    int parse_args(int argc, char ** argv);
    
    // initialize the model
    int init(int argc, char ** argv);
    
    // load LDA model to continue estimating or to do inference
    int load_model(string model_name);
    
    // save LDA model to files
    // model_name.tassign: topic assignments for words in docs
    // model_name.theta: document-topic distributions
    // model_name.phi: topic-word distributions
    // model_name.others: containing other parameters of the model (alpha, beta, M, V, K)
    int save_model(string model_name);
    int save_model_tassign(string filename);
    int save_model_theta(string filename);
    int save_model_phi(string filename);
    int save_model_others(string filename);
    int save_model_twords(string filename);
    
    // saving inference outputs
    int save_inf_model(string model_name);
    int save_inf_model_tassign(string filename);
    int save_inf_model_newtheta(string filename);
    int save_inf_model_newphi(string filename);
    int save_inf_model_others(string filename);
    int save_inf_model_twords(string filename);
    
    // init for estimation
    int init_est();
    int init_estc();
	
    // estimate LDA model using Gibbs sampling
    void estimate();
    int sampling(int m, int n, double *f1);
    void compute_theta();
    void compute_phi();
    
    // init for inference
    int init_inf();
    // inference for new (unseen) data based on the estimated LDA model
    void inference();
    int inf_sampling(int m, int n, double *f1);
    void compute_newtheta();
    void compute_newphi();
/*
 help fucntion 
*/
    template<typename D>
    bool write_to_disk(int index,vector<string> &file_names,vector<D> &object){
        //index to page
        ofstream ofs;
        ofs.open(file_names[index], ofstream::out|ofstream::trunc);
        boost::archive::text_oarchive oa(ofs);
        oa << object;
        ofs.close();
        return true;
    }

    template<typename D>
    bool read_from_disk(int index,vector<string> &file_names, vector<int> &start_index, int &cur_size, int & cur_index, vector<D>& object){
      //index to page
      if(cur_index < start_index.size()){
        write_to_disk(cur_index,file_names,object);
      }
      ifstream ifs(file_names[index]);
      boost::archive::text_iarchive ia(ifs);
      object.clear();
      ia >> object;
      cur_size = object.size();
      cur_index = index;
      return true;
    }
    
     
    template<typename D>
    D& get_docs(int idx,vector<string> &file_names, vector<int> &start_index, int &cur_size, int & cur_index, vector<D>& object) {
       if(!( cur_index < start_index.size()&&
             idx >= start_index[cur_index]&& 
            (cur_index==start_index.size()-1||idx < start_index[cur_index+1]))){
         int start = 0;
         int end = start_index.size()-1;
         while(start+1 < end){
           int mid = start + ((end-start)>>1);
            if(idx < start_index[mid]){
              end = mid;
            }else{
              start = mid;
            }
         }
         int dex = idx >= start_index[end]?end:start;
         read_from_disk(dex,file_names,start_index,cur_size,cur_index,object);
       } 
       return object[idx-start_index[cur_index]];
     }  
      
     template<typename E>
     void add_element(E&e, int idx, vector<string> &file_names, vector<int> &start_index, 
                      int &cur_size, int & cur_index, vector<E>& object,string name){
       if(0 <= idx && idx < M){
         if(object.empty()){
           start_index.push_back(idx);
         } 
         object.push_back(e);
         cur_size++;
         if(object.size() >MAX_MEM/4){
           file_names.push_back(dir+name+"_"+std::to_string(cur_index)+".data");
           write_to_disk(cur_index,file_names,object);
           object.clear();
           cur_size = 0;
           cur_index++; 
         }
       }
     }

};

#endif

