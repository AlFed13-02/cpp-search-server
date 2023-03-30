// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

void TestDocumentsWithMinusWordsExcludedFromSearchResults() {
    const int doc_id_0 = 42;
    const string content_0 = "cat in the city"s;
    const vector<int> ratings_0 = {1, 2, 3};
    
    const int doc_id_1 = 43;
    const string content_1 = "dog in the city"s;
    const vector<int> ratings_1 = {5, 6, 8};
    
    SearchServer server;
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
    
    SearchServer server;
    server.AddDocument(doc_id_0, content_0, DocumentStatus::ACTUAL, ratings_0);
    server.AddDocument(doc_id_1, content_1, DocumentStatus::ACTUAL, ratings_1);
    
    {
        const auto matched_words = get<0>(server.MatchDocument("little cat"s, doc_id_0));
        vector<string> expected_result = {"cat"s};
        ASSERT_EQUAL(matched_words, expected_result);
    }
    
    {
        const auto matched_words = get<0>(server.MatchDocument("little -cat"s, doc_id_0));
        vector<string> result = {};
        ASSERT(matched_words.empty());
    }
}

void TestDocumentsSortedByRelevance() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {4, 5, -12, 2, 1});
    
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    vector<int> doc_ids;
    for (const auto& doc: found_docs) {
        doc_ids.push_back(doc.id);
    }
    vector<int> expected_result = {1, 2, 0};
    ASSERT_EQUAL(doc_ids, expected_result);
}

void TestComputeAverageRating() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {4, 5, -12, 2, 1});
    
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    const int rating = found_docs[0].rating;
    ASSERT_EQUAL(rating, 4);
}

void TestResultsFilterUsingPredicate() {
    SearchServer server;
    server.SetStopWords("и в на"s);
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
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::BANNED, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::REMOVED, {4, 5, -12, 2, 1});
    
    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::ACTUAL);
        const int id = found_docs[0].id;
        ASSERT_EQUAL(id, 0);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
        const int id = found_docs[0].id;
        ASSERT_EQUAL(id, 1);
    }
    
    {
        const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::REMOVED);
        const int id = found_docs[0].id;
        ASSERT_EQUAL(id, 2);
    }
}

void TestCorrectRelevanceComputation() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, {2, 8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {3, 7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {4, 5, -12, 2, 1});
    
    const double EPSILON = 1e-6;
    const auto found_doc = server.FindTopDocuments("пушистый ухоженный кот"s);
    double epsilon = found_doc[0].relevance - 0.650672;
    ASSERT(epsilon < EPSILON);
    epsilon = found_doc[1].relevance - 0.274653;
    ASSERT(epsilon < EPSILON);
    epsilon = found_doc[2].relevance - 0.101366;
    ASSERT(epsilon < EPSILON);
}

void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestDocumentsWithMinusWordsExcludedFromSearchResults); 
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestDocumentsSortedByRelevance);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestResultsFilterUsingPredicate);
    RUN_TEST(TestFindTopDocumentsWithDefiniteStatus);
    RUN_TEST(TestCorrectRelevanceComputation);
}

// --------- Окончание модульных тестов поисковой системы -----------
