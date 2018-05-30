// Copyright 2018 Google Inc. All Rights Reserved.
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

#include <cmath>
#include <limits>

#include "gunit.h"
#include "src/projects.h"

#define isnan std::isnan

namespace {

TEST(PjPhi2Test, Basic) {
  projCtx ctx = pj_get_default_ctx();

  EXPECT_DOUBLE_EQ(M_PI_2, pj_phi2(ctx, 0.0, 0.0));

  EXPECT_DOUBLE_EQ(0.0, pj_phi2(ctx, 1.0, 0.0));
  EXPECT_DOUBLE_EQ(M_PI_2, pj_phi2(ctx, 0.0, 1.0));
  EXPECT_DOUBLE_EQ(M_PI, pj_phi2(ctx, -1.0, 0.0));
  EXPECT_DOUBLE_EQ(M_PI_2, pj_phi2(ctx, 0.0, -1.0));

  EXPECT_DOUBLE_EQ(0.0, pj_phi2(ctx, 1.0, 1.0));
  EXPECT_DOUBLE_EQ(M_PI, pj_phi2(ctx, -1.0, -1.0));

  // TODO(schwehr): M_PI_4, M_PI_2, M_PI, M_E
  // https://www.gnu.org/software/libc/manual/html_node/Mathematical-Constants.html

  EXPECT_DOUBLE_EQ(-0.95445818456292697, pj_phi2(ctx, M_PI, 0.0));
  EXPECT_TRUE(isnan(pj_phi2(ctx, 0.0, M_PI)));
  EXPECT_DOUBLE_EQ(4.0960508381527205, pj_phi2(ctx, -M_PI, 0.0));
  EXPECT_TRUE(isnan(pj_phi2(ctx, 0.0, -M_PI)));

  EXPECT_TRUE(isnan(pj_phi2(ctx, M_PI, M_PI)));
  EXPECT_TRUE(isnan(pj_phi2(ctx, -M_PI, -M_PI)));
}


TEST(PjPhi2Test, AvoidUndefinedBehavior) {
  auto ctx = pj_get_default_ctx();

  constexpr auto nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(isnan(pj_phi2(ctx, nan, 0.0)));
  EXPECT_TRUE(isnan(pj_phi2(ctx, 0.0, nan)));
  EXPECT_TRUE(isnan(pj_phi2(ctx, nan, nan)));

  // We do not really care about the values that follow.
  constexpr auto inf = std::numeric_limits<double>::infinity();

  EXPECT_DOUBLE_EQ(-M_PI_2, pj_phi2(ctx, inf, 0.0));
  EXPECT_TRUE(isnan(pj_phi2(ctx, 0.0, inf)));

  EXPECT_DOUBLE_EQ(4.7123889803846897, pj_phi2(ctx, -inf, 0.0));
  EXPECT_TRUE(isnan(pj_phi2(ctx, 0.0, -inf)));

  EXPECT_TRUE(isnan(pj_phi2(ctx, inf, inf)));
  EXPECT_TRUE(isnan(pj_phi2(ctx, -inf, -inf)));
}

}  // namespace
