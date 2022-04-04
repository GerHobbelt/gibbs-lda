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

#include "model.h"

using namespace std;

void show_help();

 int main(int argc, char ** argv) 
{
     model lda;
     if (lda.init(argc, argv)) 
	{
		show_help();
		return 1;
    }
    
     if (lda.model_status == MODEL_STATUS_EST || lda.model_status == MODEL_STATUS_ESTC) 
	{
		// parameter estimation
		lda.estimate();
    }
    
     if (lda.model_status == MODEL_STATUS_INF)
	{
		// do inference
		lda.inference();
    }
    return 0;
}

void show_help() 
{
    printf("Command line usage:\n");
    printf("\tlda -est -alpha <double> -beta <double> -ntopics <int> -niters <int> -savestep <int> -twords <int> -treval <1|0> -dfile <string>\n");
	printf("\t./lda -est -alpha 0.25  -beta 0.1 -ntopics 20 -niters 200 -savestep 100 -twords 50 -treval 1 -dfile  ../models/casestudy/trndocs.dat\n");

    printf("\tlda -estc -dir <string> -model <string> -niters <int> -savestep <int> -twords <int> -treval <1|0> <int> \n");
	printf("\t./lda -estc -dir ../models/casestudy/ -model model-00200 -niters 200 -savestep 100 -twords 50 -treval 1 \n");
	
	//train dataset:treval(perplexity) if 1:get it   0:not get it.
	//test  dataset:teval (perplexity) if 1:get it   0:not get it.
    printf("\tlda -inf -dir <string> -model <string> -niters <int> -twords <int> -treval <1|0> -teval <1|0>  -dfile <string>    \n");
	printf("\t./lda -inf -dir ../models/casestudy/ -model model-00400 -niters  100 -twords  50 -treval 1 -teval 1 -dfile newdocs.dat\n");

}

