// Copyright 2020 The Tint Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <memory>

#include "gtest/gtest.h"
#include "src/ast/discard_statement.h"
#include "src/ast/loop_statement.h"
#include "src/writer/wgsl/generator_impl.h"

namespace tint {
namespace writer {
namespace wgsl {
namespace {

using WgslGeneratorImplTest = testing::Test;

TEST_F(WgslGeneratorImplTest, Emit_Loop) {
  auto body = std::make_unique<ast::BlockStatement>();
  body->append(std::make_unique<ast::DiscardStatement>());
  ast::LoopStatement l(std::move(body), {});

  GeneratorImpl g;
  g.increment_indent();

  ASSERT_TRUE(g.EmitStatement(&l)) << g.error();
  EXPECT_EQ(g.result(), R"(  loop {
    discard;
  }
)");
}

TEST_F(WgslGeneratorImplTest, Emit_LoopWithContinuing) {
  auto body = std::make_unique<ast::BlockStatement>();
  body->append(std::make_unique<ast::DiscardStatement>());

  auto continuing = std::make_unique<ast::BlockStatement>();
  continuing->append(std::make_unique<ast::DiscardStatement>());

  ast::LoopStatement l(std::move(body), std::move(continuing));

  GeneratorImpl g;
  g.increment_indent();

  ASSERT_TRUE(g.EmitStatement(&l)) << g.error();
  EXPECT_EQ(g.result(), R"(  loop {
    discard;

    continuing {
      discard;
    }
  }
)");
}

}  // namespace
}  // namespace wgsl
}  // namespace writer
}  // namespace tint
