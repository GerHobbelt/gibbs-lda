//
// Created by lt on 2/5/17.
//

#ifndef GIBBSLDA_LDA_HPP
#define GIBBSLDA_LDA_HPP

#include "../include/model.h"
#include <sstream>
#include <iterator>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>

class GibbsRecommender {
public:
    void InitRecommender();
    std::vector<std::string> GetRecommendation(std::vector<std::string> clickstream, int numRec);
    int main();

private:
    model ldamodel;
};

#endif //GIBBSLDA_LDA_HPP
