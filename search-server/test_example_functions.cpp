#include <string>
#include <iostream>
#include <vector>

#include "test_example_functions.h"
#include "search_server.h"
#include "document.h"

using namespace std;

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    
    {
        SearchServer server("and the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("and the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("and"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

void TestAddDocument() {
    SearchServer server("and the"s);
    size_t doc_count = server.GetDocumentCount();
    size_t expected_result = 0;
    ASSERT_EQUAL(doc_count, expected_result);
    
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    doc_count = server.GetDocumentCount();
    expected_result = 1;
    ASSERT_EQUAL(doc_count, expected_result);
}

void TestDocumentsWithMinusWordsExcludedFromSearchResults() {
    const int doc_id_0 = 42;
    const string content_0 = "cat in the city"s;
    const vector<int> ratings_0 = {1, 2, 3};
    
    const int doc_id_1 = 43;
    const string content_1 = "dog in the city"s;
    const vector<int> ratings_1 = {5, 6, 8};
    
    SearchServer server("and the"s);
    server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    
    const auto found_docs = server.FindTopDocuments("-cat in the city"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id_1);
}

void TestMatchDocument() {
    const int doc_id_0 = 42;
    const string content_0 = "cat in the city"s;
    const vector<int> ratings_0 = {1, 2, 3};
    
    const int doc_id_1 = 43;
    const string content_1 = "dog in the city"s;
    const vector<int> ratings_1 = {5, 6, 8};
    
    SearchServer server("in the"s);
    server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    
    {
        const auto [matched_words, status] = server.MatchDocument("little cat"s, doc_id_0);
        vector<string_view> expected_result = {"cat"sv};
        ASSERT_EQUAL(matched_words, expected_result);
        ASSERT_EQUAL(static_cast<int>(status), static_cast<int>(DocumentStatus::ACTUAL));
    }
    
    {
        const auto [matched_words, status] = server.MatchDocument("little -cat"s, doc_id_0);
        ASSERT(matched_words.empty());
    }
    
    {
        const auto [matched_words, status] = server.MatchDocument("cow in the city"s, doc_id_0);
        vector<string_view> expected_result = {"city"sv};
        ASSERT_EQUAL(matched_words, expected_result);
        ASSERT_EQUAL(static_cast<int>(status), static_cast<int>(DocumentStatus::ACTUAL));
    }
}

void TestDocumentsSortedByRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {4, 5, -12, 2, 1});
    
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    for (size_t i = 1; i < found_docs.size(); ++i) {
        const double relevance0 = found_docs[i - 1].relevance;
        const double relevance1 = found_docs[i].relevance;
        ASSERT(relevance0 > relevance1);
    }
}

void TestComputeAverageRating() {
    SearchServer server("и в на"s);
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {3, 7, 2, 7});
    
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const int rating = found_docs[0].rating;
    const int expected_rating = (3 + 7 + 2 + 7) / 4;
    ASSERT_EQUAL(rating, expected_rating);
}

void TestResultsFilterUsingPredicate() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {4, 5, -12, 2, 1});
    
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, 
                                                    [](int id, DocumentStatus status, int rating){
                                                        return rating > 0;
                                                    });
    vector<int> doc_ids;
    for (const auto& doc: found_docs) {
        doc_ids.push_back(doc.id);
    }
    vector<int> expected_result = {1, 0};
    ASSERT_EQUAL(doc_ids, expected_result);
}

void TestFindTopDocumentsWithDefiniteStatus() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::REMOVED, {4, 5, -12, 2, 1});
    
    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const int id = found_docs[0].id;
        ASSERT_EQUAL(id, 0);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const int id = found_docs[0].id;
        ASSERT_EQUAL(id, 1);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::REMOVED);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const int id = found_docs[0].id;
        ASSERT_EQUAL(id, 2);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::IRRELEVANT);
        ASSERT(found_docs.empty());
    }
    
}

void TestCorrectRelevanceComputation() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {4, 5, -12, 2, 1});
    
    string query = "пушистый ухоженный кот"s;
    vector<double> idfs = {log(3. / 1), log(3. / 1), log(3. / 2)};
    vector<double> tfs_for_doc_1 = {2. / 4, 0. / 4, 1. / 4};
    double expected_relevance = 0.;
    for (int i = 0; i < 3; ++i) {
        expected_relevance += idfs[i] * tfs_for_doc_1[i];
    }
    const auto found_doc = server.FindTopDocuments(query);
    ASSERT_EQUAL(found_doc.size(), 3u);
    const double epsilon = 1e-6;
    double delta = abs(found_doc[0].relevance - expected_relevance);
    ASSERT(delta < epsilon);
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestDocumentsWithMinusWordsExcludedFromSearchResults); 
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestDocumentsSortedByRelevance);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestResultsFilterUsingPredicate);
    RUN_TEST(TestFindTopDocumentsWithDefiniteStatus);
    RUN_TEST(TestCorrectRelevanceComputation);
}
