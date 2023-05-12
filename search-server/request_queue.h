#pragma once
#include <vector>
#include <string>
#include <deque>

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        if (!requests_.empty() && min_in_day_ <= current_time_ - requests_.front().timestamp) {
            requests_.pop_front();
        }
      
        auto requested_documents = server_.FindTopDocuments(raw_query, document_predicate);
        if (requested_documents.empty()) {
            requests_.push_back(QueryResult{raw_query, current_time_});
        } 
        
        ++current_time_;
        return requested_documents;
    }
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        std::string query_text;
        int timestamp;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    int current_time_;
};
