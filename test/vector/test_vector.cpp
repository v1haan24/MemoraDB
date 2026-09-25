#include <gtest/gtest.h>
#include "vector/semantic/vector_index.h"
#include "engine/executor.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include "../test_helper.h"
#include <algorithm>
#include <cmath>

static Statement parseStmt(const std::string& sql) {
    Lexer lexer(sql);
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    return parser.parseStatement();
}

TEST(VectorTest, CosineSimilarityCalculations) {
    float a[VEC_DIM] = {};
    float b[VEC_DIM] = {};

    // Zero vectors
    EXPECT_FLOAT_EQ(cosineSimilarity(a, b), 0.0f);

    // Identical unit vectors
    a[0] = 1.0f;
    b[0] = 1.0f;
    EXPECT_NEAR(cosineSimilarity(a, b), 1.0f, 1e-5f);

    // Orthogonal vectors
    b[0] = 0.0f;
    b[1] = 1.0f;
    EXPECT_NEAR(cosineSimilarity(a, b), 0.0f, 1e-5f);

    // Opposite vectors
    b[0] = -1.0f;
    b[1] = 0.0f;
    EXPECT_NEAR(cosineSimilarity(a, b), -1.0f, 1e-5f);

    // Arbitrary vectors
    for (int i = 0; i < VEC_DIM; ++i) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i * 2);
    }
    EXPECT_NEAR(cosineSimilarity(a, b), 1.0f, 1e-5f);
}

TEST(VectorTest, SemanticSearchWithMockEmbedder) {
    TempDirectory tempDir;
    Catalog catalog;
    Executor executor(catalog);

    // Mock embedding provider that doesn't need external ONNX model files
    executor.setEmbeddingProvider([](const std::string& text, float (&out)[VEC_DIM]) -> bool {
        std::fill(std::begin(out), std::end(out), 0.0f);
        if (text.find("learning") != std::string::npos || text.find("AI") != std::string::npos) {
            out[0] = 1.0f;
        } else if (text.find("cooking") != std::string::npos || text.find("pasta") != std::string::npos) {
            out[1] = 1.0f;
        } else {
            out[2] = 1.0f;
        }
        return true;
    });

    ASSERT_TRUE(executor.execute(parseStmt(
        "CREATE TABLE docs (id INT PRIMARY KEY, title STRING(30), content STRING(200) SEMANTIC)"
    )).ok());

    ASSERT_TRUE(executor.execute(parseStmt(
        "INSERT INTO docs VALUES (1, 'Machine Learning', 'AI and deep learning methods')"
    )).ok());

    ASSERT_TRUE(executor.execute(parseStmt(
        "INSERT INTO docs VALUES (2, 'Cooking Pasta', 'delicious homemade pasta recipe')"
    )).ok());

    ASSERT_TRUE(executor.execute(parseStmt(
        "INSERT INTO docs VALUES (3, 'Gardening', 'spring gardening and planting flowers')"
    )).ok());

    // Query for AI/learning content
    ExecResult res = executor.execute(parseStmt(
        "SELECT * FROM docs WHERE content SIMILAR TO 'learning algorithms' LIMIT 2"
    ));

    ASSERT_TRUE(res.ok());
    EXPECT_EQ(res.kind, ExecResult::Kind::SEARCH);
    ASSERT_EQ(res.search.size(), 2);

    // First result should be document 1
    EXPECT_EQ(res.search[0].pk, "1");
    EXPECT_NEAR(res.search[0].score, 1.0f, 1e-5f);

    // Second result should have score 0 (orthogonal)
    EXPECT_NEAR(res.search[1].score, 0.0f, 1e-5f);
}
