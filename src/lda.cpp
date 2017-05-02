#include <chrono>
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
//    std::cout << n_gamma.size() << ", " << lambda.size() << "*" << lambda[0].size() << std::endl;
    std::vector<double> pw_q;
    calcPw_q(lambda, n_gamma, pw_q);
//    std::cout << pw_q.size() << std::endl;
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
    std::chrono::steady_clock::time_point recommendation_query, recommendation_get;
    recommendation_query = std::chrono::steady_clock::now();
    std::vector<std::string> rec = GR.GetRecommendation({"cancer", "week"}, 5);
    recommendation_get = std::chrono::steady_clock::now();
    auto rec_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(recommendation_get -
                                                                                    recommendation_query).count();
    std::cout << "Recommendations:";
    for (auto r: rec) {
        std::cout << r << ",";
    }
    std::cout << ", Rec time(ms):" << rec_seconds <<std::endl;
}