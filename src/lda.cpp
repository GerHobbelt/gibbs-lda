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

#include "lda.hpp"

void GibbsRecommender::InitRecommender() {
    string dir = "/home/lt/Downloads/GibbsLDA++-0.2/models/test/";
    string model_name = "model-final";
    int niters = 10;
    ldamodel.init(dir, model_name, niters);
}

void calcPw_q(std::vector<std::vector<double>> pw_z, std::vector<double> pz_q, std::vector<double> &pw_q) {
    std::vector<double> temp(pw_z.size());
    pw_q.swap(temp);
    int j = 0;
    for (auto &it : pw_z) {
        double sum = 0.0;
        int k = 0;
        assert(it.size() <= pz_q.size());
        for (auto &rit : it) {
            sum += rit * pz_q[k];
            ++k;
        }
        pw_q[j] = sum;
        ++j;
    }
}

namespace helper
{
    template<typename A, typename B>
    std::pair<B,A> flip_pair(const std::pair<A,B> &p)
    {
        return std::pair<B,A>(p.second, p.first);
    }
    // flips an associative container of A,B pairs to B,A pairs
    template<typename A, typename B, template<class,class,class...> class M, class... Args>
    std::multimap<B,A> flip_map(const M<A,B,Args...> &src)
    {
        std::multimap<B,A> dst;
        std::transform(src.begin(), src.end(),
                       std::inserter(dst, dst.begin()),
                       flip_pair<A,B>);
        return dst;
    }
}

std::vector<std::string> GibbsRecommender::GetRecommendation(std::vector<std::string> clickstream, int numRec) {
    std::vector<std::string> Rec;
    std::string clickstream_string = "";
    for (auto c : clickstream) {
        clickstream_string += c + "\t";
    }
    std::vector<double> n_gamma = ldamodel.get_topicdistribution(clickstream_string);
    std::vector<std::vector<double>> lambda = ldamodel.get_phi();
    std::cout << n_gamma.size() << ", " << lambda.size() << "*" << lambda[0].size() << std::endl;
    std::vector<double> pw_q;
    calcPw_q(lambda, n_gamma, pw_q);
    std::cout << pw_q.size() << std::endl;
    std::map<std::size_t,double> pmfHist;
    std::map<std::string,double> histo;
    for(std::size_t idx(0); idx < pw_q.size(); idx++)
    {
        pmfHist[idx] = pw_q.at(idx);
    }
    for(const auto & kvpair : ldamodel.mapword2id)
    {
        histo[kvpair.first] = pmfHist.at(kvpair.second);
    }
    std::multimap<double, std::string> histoInvert = helper::flip_map(histo);
    std::size_t count = 0;
    for(const auto& p : boost::adaptors::reverse(histoInvert))
    {
        if(count==numRec)
            break;
        Rec.push_back(p.second);
        count++;
    }
    return Rec;
}

int main() {
    GibbsRecommender GR;
    GR.InitRecommender();
    std::vector<std::string> rec = GR.GetRecommendation({"lawyer", "pregnant"}, 10);
    for (auto r: rec) {
        std::cout << r << ",";
    }
    std::cout << std::endl;
}