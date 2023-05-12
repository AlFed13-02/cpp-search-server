#pragma once
#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>
#include <iterator>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator range_begin, Iterator range_end):
        range_begin_(range_begin),
        range_end_(range_end),
        size_(distance(range_begin_, range_end_)) 
    {
    }
    
    Iterator begin() const {
        return range_begin_;
    }
    
    Iterator end() const {
        return range_end_;
    }
    
    size_t size() const {
        return size_;
    }
   
private:
    Iterator range_begin_;
    Iterator range_end_;
    size_t size_;
};

template <typename Iterator>
std::ostream& operator<<(std::ostream& output, const IteratorRange<Iterator>& range) {
    for (auto start = range.begin(); start != range.end(); ++start) {
        output << *start;
    }
    return output;
}


template <typename Iterator> 
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size){
        for (size_t left = std::distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back({begin, current_page_end});

            left -= current_page_size;
            begin = current_page_end;
        }
    }
    
    auto begin() const{
        return pages_.begin();
    }
    
    auto end() const{
        return pages_.end();
    }
    
    std::size_t size() const {
        return pages_.size();
    }
    
private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, std::size_t page_size) {
    return Paginator(std::begin(c), std::end(c), page_size);
}
