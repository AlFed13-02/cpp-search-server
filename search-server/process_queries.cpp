#include <vector>
#include <string>
#include <execution>
#include <algorithm>
#include <numeric>
#include <list>

#include "document.h"
#include "search_server.h"

using namespace std;

vector<vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const vector<string>& queries) {
    vector<vector<Document>> result(queries.size());
    transform(
        execution::par,
        queries.begin(),
        queries.end(),
        result.begin(),
        [&search_server](const string& query){
            return search_server.FindTopDocuments(query);
        });
    return result;
}

vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const vector<string>& queries) {
    auto query_results = ProcessQueries(search_server, queries);
    vector<Document> result = reduce(
        query_results.begin(),
        query_results.end(),
        vector<Document>(),
        [](vector<Document>& l, const vector<Document>& v) {
            l.insert(l.end(), v.begin(), v.end());
            return l;
        });
    return result;
        
}