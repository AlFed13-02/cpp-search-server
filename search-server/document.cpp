#include <iostream>
#include <string>

#include "document.h"

using namespace std;

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}

ostream& operator<<(ostream& output, const Document& doc) {
    output << "{ document_id = "s << doc.id << ", relevance = "s 
           << doc.relevance << ", rating = "s << doc.rating << " }"s;
    return output;
}