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

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <math.h>
#include <random>

#include "constants.h"
#include "strtokenizer.h"
#include "utils.h"
#include "dataset.h"
#include "model.h"

using namespace std;

model::~model()
{
	delete p;
	delete ptrndata;
	delete pnewdata;

    /*if (z) {
	for (int m = 0; m < M; m++) {
	    if (z[m]) {
		delete z[m];
	    }
	}
    }*/

	if (nw) {
		for (int w = 0; w < V; w++) {
			if (nw[w]) {
				delete nw[w];
			}
		}
	}

    /*if (nd) {
	for (int m = 0; m < M; m++) {
	    if (nd[m]) {
		delete nd[m];
	    }
	}
    } */

	delete nwsum;
	delete ndsum;

    /*if (theta) {
	for (int m = 0; m < M; m++) {
	    if (theta[m]) {
		delete theta[m];
	    }
	}
    }*/

	if (phi) {
		for (int k = 0; k < K; k++) {
			if (phi[k]) {
				delete phi[k];
			}
		}
	}

	// only for inference
	if (newz) {
		for (int m = 0; m < newM; m++) {
			if (newz[m]) {
				delete newz[m];
			}
		}
	}

	if (newnw) {
		for (int w = 0; w < newV; w++) {
			if (newnw[w]) {
				delete newnw[w];
			}
		}
	}

	if (newnd) {
		for (int m = 0; m < newM; m++) {
			if (newnd[m]) {
				delete newnd[m];
			}
		}
	}

	delete newnwsum;
	delete newndsum;

	if (newtheta) {
		for (int m = 0; m < newM; m++) {
			if (newtheta[m]) {
				delete newtheta[m];
			}
		}
	}

	if (newphi) {
		for (int k = 0; k < K; k++) {
			if (newphi[k]) {
				delete newphi[k];
			}
		}
	}
}

void model::set_default_values() 
{
	wordmapfile = "wordmap.txt";
	trainlogfile = "trainlog.txt";
	tassign_suffix = ".tassign";
	theta_suffix = ".theta";
	phi_suffix = ".phi";
	others_suffix = ".others";
	twords_suffix = ".twords";

	dir = "./";
	dfile = "trndocs.dat";
	model_name = "model-final";
	model_status = MODEL_STATUS_UNKNOWN;

	ptrndata = nullptr;
	pnewdata = nullptr;

	M = 0;
	V = 0;
	K = 100;
	alpha = 50.0 / K;
	beta = 0.1;
	niters = 2000;
	liter = 0;
	savestep = 200;
	twords = 0;
	withrawstrs = 0;

	p = nullptr;
	//z = nullptr;
    z_cur_size = 0;
    z_cur_index = 0; 
	nw = nullptr;
	//nd = nullptr;
    nd_cur_size = 0;
    nd_cur_index = 0;
	nwsum = nullptr;
	ndsum = nullptr;
	//theta = nullptr;
    theta_cur_size = 0;
    theta_cur_index = 0;
	phi = nullptr;

	newM = 0;
	newV = 0;
	newz = nullptr;
	newnw = nullptr;
	newnd = nullptr;
	newnwsum = nullptr;
	newndsum = nullptr;
	newtheta = nullptr;
	newphi = nullptr;
}

 int model::parse_args(int argc, char ** argv) 
{
	return utils::parse_args(argc, argv, this);
}

 int model::init(int argc, char ** argv) 
{
    // 1.read common config inforamtion .
     if (parse_args(argc, argv)) 
	{
		return 1;
	}
	
     if (model_status == MODEL_STATUS_EST )
	{
		// estimating the model from scratch
		 if (init_est()) 
		{
			return 1;
		}
    } 
	 else if (model_status == MODEL_STATUS_ESTC) 
	{
		// estimating the model from a previously estimated one
		if (init_estc()) 
		{
			return 1;
		}
    }
	else if (model_status == MODEL_STATUS_INF) 
	{
		// do inference
		if (init_inf()) 
		{
			return 1;
		}
	}

	return 0;
}

//load tassin file...
int model::load_model(const string &in_model_name)
{
	string filename = dir + in_model_name + tassign_suffix;
	FILE *fin = fopen(filename.c_str(), "r");
    if (!fin) 
	{
		printf("Cannot open file %s to load model!\n", filename.c_str());
		return 1;
	}

	char buff[BUFF_SIZE_LONG];
	string line;

	// allocate memory for z and ptrndata,size M*docszie()
    //z = new int*[M];
    //z = vector<vector<int> >(M);
    ptrndata = new dataset(M);
    ptrndata->V = V;		    //the size of training dataset dictionary.

	for (int i = 0; i < M; i++) 
	{
		char *pointer = fgets(buff, BUFF_SIZE_LONG, fin);
		if(!pointer) 
		{
			printf("Invalid word-topic assignment file, check the number of docs!\n");
			return 1;
		}

		line = buff;
		strtokenizer strtok(line, " \t\r\n");
		int length = strtok.count_tokens(); //the length of the words of a document.  
	
		vector<int> words;
		vector<int> topics;
		 for (int j = 0; j < length; j++)
		{
			string token = strtok.token(j); 

			strtokenizer tok(token, ":")  ;//289:140 290:83 291:73 292:133
			if (tok.count_tokens() != 2) 
			{
				printf("Invalid word-topic assignment line!\n");
				return 1;
			}

			char *endptr = nullptr;
			words.push_back((int) strtol(tok.token(0).c_str(), &endptr, 10));
			topics.push_back((int) strtol(tok.token(1).c_str(), &endptr, 10));
		}

		// allocate and add new document to the corpus
		auto pdoc = document(words);
		ptrndata->add_doc(pdoc, i);           //docs[i] = pdocs

		// assign values for z
		//z[i] = new int[topics.size()];
        get_z(i) = vector<int>(topics.size());
		for (size_t j = 0; j < topics.size(); j++) 
		{
		    get_z(i)[j] = topics[j];
		}
    }  //for i<M end  

	fclose(fin);

	return 0;
} 


int model::save_model(const string &in_model_name)
{
	if (save_model_tassign(dir + in_model_name + tassign_suffix)) 
	{
		return 1;
	}

	if (save_model_others(dir + in_model_name + others_suffix)) 
	{
		return 1;
	}

	if (save_model_theta(dir + in_model_name + theta_suffix)) 
	{
		return 1;
	}

	if (save_model_phi(dir + in_model_name + phi_suffix)) 
	{
		return 1;
	}

    if (twords > 0) 
	{
		if (save_model_twords(dir + in_model_name + twords_suffix)) 
		{
			return 1;
		}
	}

	return 0;
}

int model::save_model_tassign(const string &filename)
{
	int i, j;

	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	// write docs with topic assignments for words
    for (i = 0; i < ptrndata->M; i++) 
	{    
		for (j = 0; j < ptrndata->get_docs(i).length; j++) 
		{
	    	fprintf(fout, "%d:%d ", ptrndata->get_docs(i).words[j], get_z(i)[j]);
		}
		fprintf(fout, "\n");
	}

	fclose(fout);

	return 0;
}

int model::save_model_theta(const string &filename) 
{
    int index = 0;
    string base_name = filename+"."+to_string(index);
    FILE * fout = fopen(base_name.c_str(), "w");
	if (!fout) 
	{
		printf("Cannot open file %s to save!\n", base_name.c_str());
		return 1;
	}
    int size = 0; 
	for (int i = 0; i < M; i++) 
	{
		for (int j = 0; j < K; j++) 
		{
            ++size;
		    fprintf(fout, "%f ", get_theta(i)[j]);
		}
		fprintf(fout, "\n");
        if (size > MOD_THETA_SAVE/8)
		{
          fclose(fout );
          fout = nullptr;
          ++index;
          base_name = filename+"."+to_string(index);
          fout = fopen(base_name.c_str(),"w");
          if (!fout) 
		  {
			printf("Cannot open file %s to save!\n", base_name.c_str());
			return 1;
          }
          size = 0;
        }
	}

	fclose(fout);
    fout = nullptr; 
	return 0;
}

int model::save_model_phi(const string &filename)
{
	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	for (int i = 0; i < K; i++) {
		for (int j = 0; j < V; j++) {
			fprintf(fout, "%f ", phi[i][j]);
		}
		fprintf(fout, "\n");
	}

	fclose(fout);

	return 0;
}

int model::save_model_others(const string &filename)
{
	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	fprintf(fout, "alpha=%f\n", alpha);
	fprintf(fout, "beta=%f\n", beta);
	fprintf(fout, "ntopics=%d\n", K);
	fprintf(fout, "ndocs=%d\n", M);
	fprintf(fout, "nwords=%d\n", V);
	fprintf(fout, "liter=%d\n", liter);

	fclose(fout);

	return 0;
}

int model::save_model_twords(const string &filename)
{
	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) 
	{
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	printf("Number of most likely words per topic: %i\n", twords);
	if (twords > V) 
	{
		printf("Set number of most likely words to V: %i\n", V);
		twords = V;
	}
	mapid2word::iterator it;

#if 0
    if (twords > 0) 
	{
    	dataset::read_wordmap(dir + wordmapfile, &id2word);
    }
#endif
    printf("Size of id2word: %zu\n", id2word.size());

	if (id2word.empty()) {
		printf("Something went wrong with loading the words, size of id2word: %zu\n", id2word.size());
		return -1;
	}

	for (int k = 0; k < K; k++) 
	{
		vector<pair<int, double> > words_probs;
		pair<int, double> word_prob;
		for (int w = 0; w < V; w++) 
		{
			word_prob.first = w;
			word_prob.second = phi[k][w];
			words_probs.push_back(word_prob);
		}

		// quick sort to sort word-topic probability
		utils::quicksort(words_probs, 0, (int) words_probs.size() - 1);

		fprintf(fout, "Topic %dth:\n", k);
		for (int i = 0; i < twords; i++) 
		{
			it = id2word.find(words_probs[i].first);
			if (it != id2word.end()) 
			{
				fprintf(fout, "\t%s   %f\n", (it->second).c_str(), words_probs[i].second);
			}
		}
	}

	fclose(fout);

	return 0;
}

int model::save_inf_model(const string &in_model_name) 
{
	if (save_inf_model_tassign(dir + in_model_name + tassign_suffix)) 
	{
		return 1;
	}

	if (save_inf_model_others(dir + in_model_name + others_suffix)) 
	{
		return 1;
	}

	if (save_inf_model_newtheta(dir + in_model_name + theta_suffix)) 
	{
		return 1;
	}

	if (save_inf_model_newphi(dir + in_model_name + phi_suffix)) 
	{
		return 1;
	}

	if (twords > 0) 
	{
		if (save_inf_model_twords(dir + in_model_name + twords_suffix)) 
		{
			return 1;
		}
	}

	return 0;
}

int model::save_inf_model_tassign(const string &filename) 
{
	int i, j;

	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	// write docs with topic assignments for words
	for (i = 0; i < pnewdata->M; i++) 
	{
		for (j = 0; j < pnewdata->docs[i].length; j++) 
		{
		    fprintf(fout, "%d:%d ", pnewdata->docs[i].words[j], newz[i][j]);
		}
		fprintf(fout, "\n");
	}

	fclose(fout);

	return 0;
}

int model::save_inf_model_newtheta(const string &filename) 
{
	int i, j;

	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	for (i = 0; i < newM; i++) {
		for (j = 0; j < K; j++) {
			fprintf(fout, "%f ", newtheta[i][j]);
		}
		fprintf(fout, "\n");
	}

	fclose(fout);

	return 0;
}

int model::save_inf_model_newphi(const string &filename) 
{
	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	for (int i = 0; i < K; i++) {
		for (int j = 0; j < newV; j++) {
			fprintf(fout, "%f ", newphi[i][j]);
		}
		fprintf(fout, "\n");
	}

	fclose(fout);

	return 0;
}

int model::save_inf_model_others(const string &filename) 
{
	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	fprintf(fout, "alpha=%f\n", alpha);
	fprintf(fout, "beta=%f\n", beta);
	fprintf(fout, "ntopics=%d\n", K);
	fprintf(fout, "ndocs=%d\n", newM);
	fprintf(fout, "nwords=%d\n", newV);
	fprintf(fout, "liter=%d\n", inf_liter);

	fclose(fout);

	return 0;
}

int model::save_inf_model_twords(const string &filename) 
{
	FILE *fout = fopen(filename.c_str(), "w");
	if (!fout) {
		printf("Cannot open file %s to save!\n", filename.c_str());
		return 1;
	}

	if (twords > newV) {
		twords = newV;
	}
	mapid2word::iterator it;
	map<int, int>::iterator _it;

	for (int k = 0; k < K; k++) {
		vector<pair<int, double> > words_probs;
		pair<int, double> word_prob;
		for (int w = 0; w < newV; w++) {
			word_prob.first = w;
			word_prob.second = newphi[k][w];
			words_probs.push_back(word_prob);
		}

		// quick sort to sort word-topic probability
		utils::quicksort(words_probs, 0, words_probs.size() - 1);

		fprintf(fout, "Topic %dth:\n", k);
		for (int i = 0; i < twords; i++) {
			_it = pnewdata->_id2id.find(words_probs[i].first);
			if (_it == pnewdata->_id2id.end()) {
				continue;
			}
			it = id2word.find(_it->second);
			if (it != id2word.end()) {
				fprintf(fout, "\t%s   %f\n", (it->second).c_str(), words_probs[i].second);
			}
		}
	}

	fclose(fout);

	return 0;
}


int read_wordmap(const string &wordmapfile, mapword2id *pword2id) 
{
    pword2id->clear();

    FILE *fin = fopen(wordmapfile.c_str(), "r");
    if (!fin) 
	{
        printf("Cannot open file %s to read!\n", wordmapfile.c_str());
        return 1;
    }

    char buff[BUFF_SIZE_SHORT];
    string line;

    fgets(buff, BUFF_SIZE_SHORT - 1, fin);
    int nwords = atoi(buff);

    for (int i = 0; i < nwords; i++) 
	{
        fgets(buff, BUFF_SIZE_SHORT - 1, fin);
        line = buff;

        strtokenizer strtok(line, " \t\r\n");
        if (strtok.count_tokens() != 2) 
		{
            continue;
        }

        pword2id->insert(pair<string, int>(strtok.token(0), atoi(strtok.token(1).c_str())));
    }

    fclose(fin);

    return 0;
}


static std::mt19937 rnd_gen;
static std::uniform_real_distribution<> rnd_distr;


int model::init_est()
{
	int m, n, w, k;

	p = new double[K];

	// + read training data
	ptrndata = new dataset;
    ptrndata->dir = dir;
	if (ptrndata->read_trndata(dir + dfile, dir + wordmapfile)) 
	{
		printf("Fail to read training data!\n");
		return 1;
	}

	// + allocate memory and assign values for variables
	M = ptrndata->M;
	V = ptrndata->V;
	// K: from command line or default value
	// alpha, beta: from command line or default values
	// niters, savestep: from command line or default values

	nw = new int *[V];
	for (w = 0; w < V; w++) 
	{
		nw[w] = new int[K];
		for (k = 0; k < K; k++) 
		{
			nw[w][k] = 0;
		}
	}

    //nd = vector<vector<int> >(M);
	for (m = 0; m < M; m++) 
	{
        auto nd_e = vector<int>(K);
		for (k = 0; k < K; k++) 
		{
            nd_e[k] = 0;
		}
        add_nd(nd_e,m);
	}

    if (nd.size() > 0)
	{
#ifdef HAVE_BOOST
		nd_file_names.push_back(dir+"ND_"+std::to_string(nd_cur_index)+".data");
      write_to_disk(nd_cur_index,nd_file_names,nd) ; 
      //nd.clear();
      nd_cur_size = 0;
      nd_cur_index++;
#else
#endif
	}

	nwsum = new int[K];
	for (k = 0; k < K; k++) 
	{
		nwsum[k] = 0;
	}

	ndsum = new int[M];
	for (m = 0; m < M; m++) 
	{
		ndsum[m] = 0;
	}


#if 0
	srandom(time(nullptr)); // initialize for random number generation
#else
	// uncomment to use a non-deterministic seed
	//    std::random_device rd;
	//    rnd_gen = std::mt19937(rd());
	rnd_gen = std::mt19937(1729);
	rnd_distr = std::uniform_real_distribution<>(0.0, 1.0);
#endif

    //z = vector<vector<int>>(M);
	for (m = 0; m < ptrndata->M; m++) 
	{
		document &mydoc = ptrndata->get_docs(m);
		int N = mydoc.length;
        vector<int> z_e = vector<int>(N,0);

		// initialize for z
		for (n = 0; n < N; n++) 
		{
#if 0
			int topic = (int)(((double)random() / RAND_MAX) * K); // random()/RAND_MAX ---> [0,1>
#else
			int topic = (int)(rnd_distr(rnd_gen) * K); // random()/RAND_MAX ---> [0,1>
#endif
			z_e[n] = topic;

			// number of instances of word i assigned to topic j
    	    nw[mydoc.words[n]][topic] += 1;					 
			// number of words in document i assigned to topic j
            get_nd(m)[topic] += 1;
    	    //nd[m][topic] += 1;
			// total number of words assigned to topic j
			nwsum[topic] += 1;
		}
        add_z(z_e,m);
		// total number of words in document i
		ndsum[m] = N;
	}
    if (z.size() > 0)
	{
#ifdef HAVE_BOOST
		z_file_names.push_back(dir+"Z_"+std::to_string(z_cur_index)+".data");
      write_to_disk(z_cur_index,z_file_names,z) ; 
      //nd.clear();
      z_cur_size = 0;
      z_cur_index++;
#else
#endif
	}

    //theta = vector<vector<double> >(M);
	for (m = 0; m < M; m++) 
	{
        auto theta_e = vector<double>(K);
        add_theta(theta_e, m);
	}
    if (theta.size() > 0)
	{
#ifdef HAVE_BOOST
		theta_file_names.push_back(dir+"THETA_"+std::to_string(theta_cur_index)+".data");
      write_to_disk(theta_cur_index,theta_file_names,theta) ; 
      //nd.clear();
      theta_cur_size = 0;
      theta_cur_index++;
#else
#endif
	}
	
	phi = new double *[K];
	for (k = 0; k < K; k++) 
	{
		phi[k] = new double[V];
	}

	return 0;
}

int model::init_estc() 
{
	// estimating the model from a previously estimated one
	int m, n, w, k;

	p = new double[K];

	// load model, i.e., read z and ptrndata
	if (load_model(model_name)) 
	{
		printf("Fail to load word-topic assignment file of the model!\n");
		return 1;
	}

	nw = new int *[V];
	for (w = 0; w < V; w++) 
	{
		nw[w] = new int[K];
		for (k = 0; k < K; k++) 
		{
			nw[w][k] = 0;
		}
	}

    nd = vector<vector<int> >(M);
    //nd = new int*[M];
	for (m = 0; m < M; m++) 
	{
        //nd[m] = new int[K];
        nd[m] = vector<int>(K);
		for (k = 0; k < K; k++) 
		{
			nd[m][k] = 0;
		}
	}

	nwsum = new int[K];
	for (k = 0; k < K; k++) 
	{
		nwsum[k] = 0;
	}

	ndsum = new int[M];
	for (m = 0; m < M; m++) 
	{
		ndsum[m] = 0;
	}

	for (m = 0; m < ptrndata->M; m++) 
	{
		document &mydoc = ptrndata->get_docs(m);
		int N = mydoc.length;

		// assign values for nw, nd, nwsum, and ndsum
		for (n = 0; n < N; n++) 
		{
    	    int w = mydoc.words[n];
			int topic = z[m][n];

			// number of instances of word i assigned to topic j
    	    nw[w][topic] += 1;
			// number of words in document i assigned to topic j
			nd[m][topic] += 1;
			// total number of words assigned to topic j
			nwsum[topic] += 1;
		}
		// total number of words in document i
		ndsum[m] = N;
	}

    theta = vector<vector<double>>(M);	
    //theta = new double*[M];
	for (m = 0; m < M; m++) 
	{
        //theta[m] = new double[K];
        theta[m] = vector<double>(K);
	}

	phi = new double *[K];
	for (k = 0; k < K; k++) 
	{
		phi[k] = new double[V];
	}

	return 0;
}

//estimate from scratch and continue estimate...
void model::estimate() 
{
    if (twords > 0) 
	{
		// print out top words per topic,
		// uncertain for the model of est or estc
		// used for get topk words for each topic.
		dataset::read_wordmap(dir + wordmapfile, &id2word);
	}

	printf("Sampling %d iterations!\n", niters);

	double perplexity_result = 0.0;

    // Precalculation of loop invariants
    double Vbeta = V * beta;
    double Kalpha = K * alpha;

    // Allocate space for array of precomputed results
    double *f1;
    f1 = (double *) malloc(sizeof(double) * K);

    int last_iter = liter;  // est scratch: liter=0, estc: liter=max gibbs sampling times. for example :liter=1000
    for (liter = last_iter + 1; liter <= niters + last_iter; liter++) 
	{
		perplexity_result = 0.0;

		printf("Iteration %d ...\n", liter);

		// for all z_i
		for (int m = 0; m < M; m++) 
		{
            // Precompute results to be used in model::sampling
		    for (int k = 0; k < K; k++) 
			{
	             f1[k] = (get_nd(m)[k] + alpha) / ((nwsum[k] + Vbeta)*(ndsum[m] - 1.0 + Kalpha));
		    }

		    for (int n = 0; n < ptrndata->get_docs(m).length; n++)
			{
				// (z_i = z[m][n])
				// sample from p(z_i|z_-i, w)
				int topic = sampling(m, n, f1);
	  	        get_z(m)[n] = topic;
			}
		}
	
		if(treval)
        {    
            compute_theta();
            compute_phi();
            perplexity_result = train_perplexity();
            printf("%f\n",perplexity_result);
        }    	
	
		if (savestep > 0)
		{
			if (liter % savestep == 0)
			{
				// saving the model
				printf("Saving the model at iteration %d ...\n", liter);
				compute_theta();
				compute_phi();
				save_model(utils::generate_model_name(liter));
			}
		}
	}

	printf("Gibbs sampling completed!\n");
	printf("Saving the final model!\n");
	compute_theta();
	compute_phi();
	liter--;
	save_model(utils::generate_model_name(-1));
    free(f1);
}

/**
 * This demonstrates the nature of Gibbs sampling, namely some pop and push action on a stack (of counting variables).
 * 
 * @class_param z[][]
 *   id of topic for document index "m" and word index "n", called the "topic assignment" variable 
 * @class_param nw[][]
 *   frequency of word "w" in topic "topic"
 * @class_param nd[][]
 *   frequency of document "m" in topic "topic"
 * @class_param nwsum[]
 *   total number of words in topic "topic"
 * @class_param V
 *   total number of unique words in all topics, called "vocabulary"
 * @class_param K
 *   total number of topics
 *
 * We sample a topic z_i from the full conditional probability distribution:
 *   p(z_i=j | z_\i, w) = 
 *     (n_\i, j(w_i) + \beta) / (n_\i, j(.) + V * \beta) * 
 *     (n_\i, j(d_i) + \alpha) / (n_\i, .(d_i) + K * \alpha)
 *
 * Here z_\i indicates all z except for z_i.
 *
 * @param m
 *   document, index of the document to sample
 * @param n
 *   word, index of the word to sample (does not need to occur in the document)
 */
int model::sampling(const int m, int n, double *f1)
{
	// remove z_i from the count variables
    int topic = get_z(m)[n];
    int w = ptrndata->get_docs(m).words[n];
	nw[w][topic] -= 1;
    get_nd(m)[topic] -= 1;
	nwsum[topic] -= 1;

	double Vbeta = V * beta;
	double Kalpha = K * alpha;

    // Update only element [topic] of array f1
    f1[topic] = (get_nd(m)[topic] + alpha) / ((nwsum[topic] + Vbeta)*(ndsum[m] - 1.0 + Kalpha));

	// do multinomial sampling via cumulative method
    for (int k = 0; k < K; k++) 
	{
      // Use precomputed array f1 to avoid unnecessary calculations
        p[k] = (nw[w][k] + beta) * f1[k];
    }

    // cumulate multinomial parameters, sum will be in p[K-1]

    // Calculate sum over array p and use for scaling
    double sum=0.0;
    for (int k = 0; k < K; k++) 
	{
      sum += p[k];
    }

	// scaled sample because of unnormalized p[]

#if 0
	double u = ((double)random() / RAND_MAX) * sum;
#else
	double u = rnd_distr(rnd_gen) * sum;
#endif

    // get topic with probability beyond u 
		
    // Calculate multinomial coefficients as needed
    if(p[0] > u) 
	{
      topic = 0;
    } 
	else 
	{
      for (topic = 1; topic < K; topic++) 
	  {
        p[topic] += p[topic-1];
        if (p[topic] > u) 
		{
          break;
        }
      }
    }

	// add newly estimated z_i to count variables
	nw[w][topic] += 1;
    get_nd(m)[topic] += 1;
	nwsum[topic] += 1;

    // Recover original value of element [topic] of array f1
    f1[topic] = (get_nd(m)[topic] + alpha) / ((nwsum[topic] + Vbeta)*(ndsum[m] - 1.0 + Kalpha));
    
	return topic;
}

void model::compute_theta() 
{
    for (int m = 0; m < M; m++) 
	{
		for (int k = 0; k < K; k++) 
		{
		    get_theta(m)[k] = (get_nd(m)[k] + alpha) / (ndsum[m] + K * alpha);
		}
	}
}

void model::compute_phi() 
{
    for (int k = 0; k < K; k++) 
	{
		for (int w = 0; w < V; w++) 
		{
			phi[k][w] = (nw[w][k] + beta) / (nwsum[k] + V * beta);
		}
	}
}

int model::init_inf() 
{
	// estimating the model from a previously estimated one
	//int m, n, w, k;

    printf("Initializing the recommender.\n");

	p = new double[K];

	// load moel, i.e., read z and ptrndata
    if (load_model(model_name)) 
	{
		printf("Fail to load word-topic assignmetn file of the model!\n");
		return 1;
	}

	nw = new int *[V];
	for (int w = 0; w < V; w++) 
	{
		nw[w] = new int[K];
		for (int k = 0; k < K; k++) 
		{
			nw[w][k] = 0;
		}
	}

    nd = vector<vector<int>>(M);	
    //nd = new int*[M];
	for (int m = 0; m < M; m++)
	{ 
        nd[m] = vector<int>(K);
        //nd[m] = new int[K];
		for (int k = 0; k < K; k++) 
		{
			nd[m][k] = 0;
		}
	}

	nwsum = new int[K];
	for (int k = 0; k < K; k++) 
	{
		nwsum[k] = 0;
	}

	ndsum = new int[M];
	for (int m = 0; m < M; m++) 
	{
		ndsum[m] = 0;
	}

	for (int m = 0; m < ptrndata->M; m++) 
	{
		document &mydoc = ptrndata->get_docs(m);
		int N = mydoc.length;

		// assign values for nw, nd, nwsum, and ndsum
		for (int n = 0; n < N; n++) 
		{
    	    int w = mydoc.words[n];
			int topic = z[m][n];

			// number of instances of word i assigned to topic j
    	    nw[w][topic] += 1;
			// number of words in document i assigned to topic j
			nd[m][topic] += 1;
			// total number of words assigned to topic j
			nwsum[topic] += 1;
		}
		// total number of words in document i
		ndsum[m] = N;
	}

	// read new data for inference
	pnewdata = new dataset;
	if (withrawstrs) 
	{
		if (pnewdata->read_newdata_withrawstrs(dir + dfile, dir + wordmapfile)) 
		{
			printf("Fail to read new data!\n");
			return 1;
		}
	} 
	else 
	{
		if (pnewdata->read_newdata(dir + dfile, dir + wordmapfile)) 
		{
			printf("Fail to read new data!\n");
			return 1;
		}
	}

	newM = pnewdata->M;
	newV = pnewdata->V;

	newnw = new int *[newV];
	for (int w = 0; w < newV; w++) 
	{
		newnw[w] = new int[K];
		for (int k = 0; k < K; k++) 
		{
			newnw[w][k] = 0;
		}
	}

	newnd = new int *[newM];
	for (int m = 0; m < newM; m++) 
	{
		newnd[m] = new int[K];
		for (int k = 0; k < K; k++) 
		{
			newnd[m][k] = 0;
		}
	}

	newnwsum = new int[K];
	for (int k = 0; k < K; k++) 
	{
		newnwsum[k] = 0;
	}

	newndsum = new int[newM];
	for (int m = 0; m < newM; m++) 
	{
		newndsum[m] = 0;
	}

#if 0
	srandom(time(nullptr)); // initialize for random number generation
#else
	// uncomment to use a non-deterministic seed
	//    std::random_device rd;
	//    rnd_gen = std::mt19937(rd());
	rnd_gen = std::mt19937(1729);
	rnd_distr = std::uniform_real_distribution<>(0.0, 1.0);
#endif

	newz = new int *[newM];
	for (int m = 0; m < pnewdata->M; m++) 
	{
		document *_mydoc = pnewdata->_docs[m];
		int N = _mydoc->length;
		newz[m] = new int[N];

		// assign values for nw, nd, nwsum, and ndsum
		for (int n = 0; n < N; n++) 
		{
    	    int _w = _mydoc->words[n]; 
#if 0
			int topic = (int)(((double)random() / RAND_MAX) * K); // random()/RAND_MAX ---> [0,1>
#else
			int topic = (int)(rnd_distr(rnd_gen) * K); // random()/RAND_MAX ---> [0,1>
#endif

			newz[m][n] = topic;

			// number of instances of word i assigned to topic j
    	    newnw[_w][topic] += 1;
			// number of words in document i assigned to topic j
			newnd[m][topic] += 1;
			// total number of words assigned to topic j
			newnwsum[topic] += 1;
		}
		// total number of words in document i
		newndsum[m] = N;
	}

	// Pay attention to allocate memory for newtheta and newphi
	newtheta = new double *[newM];
	for (int m = 0; m < newM; m++) 
	{
		newtheta[m] = new double[K];
	}

	newphi = new double *[K];
	for (int k = 0; k < K; k++) 
	{
		newphi[k] = new double[newV];
	}

	return 0;
}

/* UnSeen Data inference */
void model::inference() 
{
	if (twords > 0) 
	{
		// print out top words per topic
		dataset::read_wordmap(dir + wordmapfile, &id2word);
	}

	printf("Sampling %d iterations for inference!\n", niters);

    // Precalculation of loop invariants
    double Vbeta = V * beta;
    double Kalpha = K * alpha;

    // Allocate space for array of precomputed results
    double *f1;
    f1 = (double *) malloc(sizeof(double) * K);
	
    double result=0.0;
	for (inf_liter = 1; inf_liter <= niters; inf_liter++) 
	{
		double perplexity_result =0.0;

		printf("Iteration %d ...\n", inf_liter);

		// for all newz_i
		for (int m = 0; m < newM; m++) 
		{
            // Precompute results to be used in model::inf_sampling
		    for (int k = 0; k < K; k++) 
			{
				f1[k] = (newnd[m][k] + alpha) /
					((nwsum[k] + newnwsum[k] + Vbeta)*(newndsum[m] - 1.0 + Kalpha));
		    }

			for (int n = 0; n < pnewdata->docs[m].length; n++) 
			{
				// (newz_i = newz[m][n])
				// sample from p(z_i|z_-i, w)
				int topic = inf_sampling(m, n, f1);
				newz[m][n] = topic;
			}
		}
	
		if(teval)
		{
			compute_newtheta();
			compute_newphi();
			perplexity_result = test_perplexity();

			printf("Perplexity is %f!\n",perplexity_result);
		}
    }
    
	printf("Gibbs sampling for inference completed!\n");
	printf("Saving the inference outputs!\n");
	compute_newtheta();
	compute_newphi();
	inf_liter--;
	save_inf_model(dfile);
    free(f1);
}

int model::inf_sampling(int m, int n, double *f1) 
{
	// remove z_i from the count variables
	int topic = newz[m][n];
    int w = pnewdata->docs[m].words[n];
	int _w = pnewdata->_docs[m]->words[n];
	newnw[_w][topic] -= 1;
	newnd[m][topic] -= 1;
	newnwsum[topic] -= 1;

	double Vbeta = V * beta;
	double Kalpha = K * alpha;

    // Update only element [topic] of array f1
    f1[topic] = (newnd[m][topic] + alpha) /
      ((nwsum[topic] + newnwsum[topic] + Vbeta)*(newndsum[m] - 1.0 + Kalpha));

	// do multinomial sampling via cumulative method
	for (int k = 0; k < K; k++) 
	{
		// Use precomputed array f1 to avoid unnecessary calculations
		p[k] = (nw[w][k] + newnw[_w][k] + beta) * f1[k];
	}

	// cumulate multinomial parameters

    // Calculate sum over array p and use for scaling
    double sum=0.0;
    for (int k = 0; k < K; k++) 
	{
      sum += p[k];
    }
	// scaled sample because of unnormalized p[]

#if 0
	double u = ((double)random() / RAND_MAX) * sum;
#else
	double u = rnd_distr(rnd_gen) * sum;
#endif

    // Calculate multinomial coefficients as needed
    if(p[0] > u) 
	{
      topic = 0;
    } else 
	{
      for (topic = 1; topic < K; topic++) 
	  {
        p[topic] += p[topic-1];
        if (p[topic] > u) 
		{
          break;
        }
      }
    }

	// add newly estimated z_i to count variables
	newnw[_w][topic] += 1;
	newnd[m][topic] += 1;
	newnwsum[topic] += 1;

    // Recover original value of element [topic] of array f1
    f1[topic] = (newnd[m][topic] + alpha) /
      ((nwsum[topic] + newnwsum[topic] + Vbeta)*(newndsum[m] - 1.0 + Kalpha));

	return topic;
}

void model::compute_newtheta() 
{
	for (int m = 0; m < newM; m++) 
	{
		for (int k = 0; k < K; k++) 
		{
			newtheta[m][k] = (newnd[m][k] + alpha) / (newndsum[m] + K * alpha);
		}
	}
}

void model::compute_newphi()
{
	map<int, int>::iterator it;
	for (int k = 0; k < K; k++) 
	{
		for (int w = 0; w < newV; w++) 
		{
			it = pnewdata->_id2id.find(w);
			if (it != pnewdata->_id2id.end()) 
			{
				newphi[k][w] = (nw[it->second][k] + newnw[w][k] + beta) / (nwsum[k] + newnwsum[k] + V * beta);
			}
		}
	}
}
  
double model::train_perplexity()
{
    double result       = 0.0 ;
    double expindex     = 0.0 ;
    double wordcount    =  0  ; // the words count in the whole documents.
    int m,n,k;
    for(k=0;k<K;k++)
	{
        p[k] = 0;
	}

     for ( m = 0;m < M; m++)
    {
         document &mydoc = ptrndata->get_docs(m);
         for ( n = 0; n < mydoc.length; n++)
        {
            int v =  mydoc.words[n];
             for( k=0 ; k<K ; k++ )
			 {
                p[k] = theta[m][k]*phi[k][v];
				}

            for( k = 1; k < K; k++)
			{
                p[k] += p[k-1];
				}

            expindex += log(p[k-1]);
        }
        wordcount+=ndsum[m];
	}
    result = exp(-expindex/wordcount);
    return result;
}


double model::test_perplexity()
{ 
    double result       = 0.0 ;
    double expindex  = 0.0    ;
    double wordcount    =  0  ;		//thewords count in the whole documents.
	int m,n,k;
	for(k=0;k<K;k++)
	{
		p[k] = 0 ;
	}

     for ( m = 0;m < newM; m++)
    {
		 document * _mydoc = pnewdata->_docs[m];
         for ( n = 0; n < _mydoc->length; n++)
        {
            int v =  _mydoc->words[n];
             for( k=0 ; k<K ; k++ )
			 {
                p[k] = newtheta[m][k]*newphi[k][v];
				}

            for( k = 1; k < K; k++)
			{
                p[k] += p[k-1];
				}

            expindex += log(p[k-1]);
        }
        wordcount+=ndsum[m];
    }
    result = exp(-expindex/wordcount);
    return result; 
}

