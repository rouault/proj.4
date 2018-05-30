// Copyright 2017 Google Inc. All Rights Reserved.
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
//
// TODO(schwehr): Use the proj 5 API:
//   https://github.com/OSGeo/proj.4/commit/1f096c9f9cf1ef4c152c53605bf6f6347209


#include "gunit.h"
// TODO(schwehr): Add proj.h.
// #include "proj.h"  // Avoid type pruning.
#include "projects.h"

namespace {

TEST(PJGeosTest, FuzzerCrash_3a5fb0b3f4f568f08187a0b370226469fbc36324) {
  // This fuzzer artifact crashes with:
  //   https://github.com/OSGeo/proj.4/commit/664577ced6a8e4074b1f53af82b5ae5d1d
  const char kProj[] =
      "+proj=geos +ellps=GRS80 +lat_1=0.5 +sweep=2 os +h=35785831";

  projPJ p = pj_init_plus(kProj);
  ASSERT_EQ(p, nullptr);
}

// TODO(schwehr): Add more test cases.

}  // namespace
