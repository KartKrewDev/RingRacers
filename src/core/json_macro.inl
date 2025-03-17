//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-FileCopyrightText: 2016-2021 Evan Nemerson <evan@nemerson.com>
// SPDX-License-Identifier: MIT

// file: macro_scope.hpp

// This file is derived from nlohmman json's codegen macros and thus is provided under its license.

#define SRB2_JSON_EXPAND( x ) x
#define SRB2_JSON_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, NAME,...) NAME
#define SRB2_JSON_PASTE(...) SRB2_JSON_EXPAND(SRB2_JSON_GET_MACRO(__VA_ARGS__, \
        SRB2_JSON_PASTE64, \
        SRB2_JSON_PASTE63, \
        SRB2_JSON_PASTE62, \
        SRB2_JSON_PASTE61, \
        SRB2_JSON_PASTE60, \
        SRB2_JSON_PASTE59, \
        SRB2_JSON_PASTE58, \
        SRB2_JSON_PASTE57, \
        SRB2_JSON_PASTE56, \
        SRB2_JSON_PASTE55, \
        SRB2_JSON_PASTE54, \
        SRB2_JSON_PASTE53, \
        SRB2_JSON_PASTE52, \
        SRB2_JSON_PASTE51, \
        SRB2_JSON_PASTE50, \
        SRB2_JSON_PASTE49, \
        SRB2_JSON_PASTE48, \
        SRB2_JSON_PASTE47, \
        SRB2_JSON_PASTE46, \
        SRB2_JSON_PASTE45, \
        SRB2_JSON_PASTE44, \
        SRB2_JSON_PASTE43, \
        SRB2_JSON_PASTE42, \
        SRB2_JSON_PASTE41, \
        SRB2_JSON_PASTE40, \
        SRB2_JSON_PASTE39, \
        SRB2_JSON_PASTE38, \
        SRB2_JSON_PASTE37, \
        SRB2_JSON_PASTE36, \
        SRB2_JSON_PASTE35, \
        SRB2_JSON_PASTE34, \
        SRB2_JSON_PASTE33, \
        SRB2_JSON_PASTE32, \
        SRB2_JSON_PASTE31, \
        SRB2_JSON_PASTE30, \
        SRB2_JSON_PASTE29, \
        SRB2_JSON_PASTE28, \
        SRB2_JSON_PASTE27, \
        SRB2_JSON_PASTE26, \
        SRB2_JSON_PASTE25, \
        SRB2_JSON_PASTE24, \
        SRB2_JSON_PASTE23, \
        SRB2_JSON_PASTE22, \
        SRB2_JSON_PASTE21, \
        SRB2_JSON_PASTE20, \
        SRB2_JSON_PASTE19, \
        SRB2_JSON_PASTE18, \
        SRB2_JSON_PASTE17, \
        SRB2_JSON_PASTE16, \
        SRB2_JSON_PASTE15, \
        SRB2_JSON_PASTE14, \
        SRB2_JSON_PASTE13, \
        SRB2_JSON_PASTE12, \
        SRB2_JSON_PASTE11, \
        SRB2_JSON_PASTE10, \
        SRB2_JSON_PASTE9, \
        SRB2_JSON_PASTE8, \
        SRB2_JSON_PASTE7, \
        SRB2_JSON_PASTE6, \
        SRB2_JSON_PASTE5, \
        SRB2_JSON_PASTE4, \
        SRB2_JSON_PASTE3, \
        SRB2_JSON_PASTE2, \
        SRB2_JSON_PASTE1)(__VA_ARGS__))
#define SRB2_JSON_PASTE2(func, v1) func(v1)
#define SRB2_JSON_PASTE3(func, v1, v2) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE2(func, v2)
#define SRB2_JSON_PASTE4(func, v1, v2, v3) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE3(func, v2, v3)
#define SRB2_JSON_PASTE5(func, v1, v2, v3, v4) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE4(func, v2, v3, v4)
#define SRB2_JSON_PASTE6(func, v1, v2, v3, v4, v5) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE5(func, v2, v3, v4, v5)
#define SRB2_JSON_PASTE7(func, v1, v2, v3, v4, v5, v6) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE6(func, v2, v3, v4, v5, v6)
#define SRB2_JSON_PASTE8(func, v1, v2, v3, v4, v5, v6, v7) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE7(func, v2, v3, v4, v5, v6, v7)
#define SRB2_JSON_PASTE9(func, v1, v2, v3, v4, v5, v6, v7, v8) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE8(func, v2, v3, v4, v5, v6, v7, v8)
#define SRB2_JSON_PASTE10(func, v1, v2, v3, v4, v5, v6, v7, v8, v9) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE9(func, v2, v3, v4, v5, v6, v7, v8, v9)
#define SRB2_JSON_PASTE11(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE10(func, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define SRB2_JSON_PASTE12(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE11(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
#define SRB2_JSON_PASTE13(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE12(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
#define SRB2_JSON_PASTE14(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE13(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
#define SRB2_JSON_PASTE15(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE14(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
#define SRB2_JSON_PASTE16(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE15(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
#define SRB2_JSON_PASTE17(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE16(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)
#define SRB2_JSON_PASTE18(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE17(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17)
#define SRB2_JSON_PASTE19(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE18(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18)
#define SRB2_JSON_PASTE20(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE19(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19)
#define SRB2_JSON_PASTE21(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE20(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20)
#define SRB2_JSON_PASTE22(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE21(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21)
#define SRB2_JSON_PASTE23(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE22(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22)
#define SRB2_JSON_PASTE24(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE23(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23)
#define SRB2_JSON_PASTE25(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE24(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24)
#define SRB2_JSON_PASTE26(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE25(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25)
#define SRB2_JSON_PASTE27(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE26(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26)
#define SRB2_JSON_PASTE28(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE27(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27)
#define SRB2_JSON_PASTE29(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE28(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)
#define SRB2_JSON_PASTE30(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE29(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)
#define SRB2_JSON_PASTE31(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE30(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)
#define SRB2_JSON_PASTE32(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE31(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)
#define SRB2_JSON_PASTE33(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE32(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32)
#define SRB2_JSON_PASTE34(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE33(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33)
#define SRB2_JSON_PASTE35(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE34(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34)
#define SRB2_JSON_PASTE36(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE35(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35)
#define SRB2_JSON_PASTE37(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE36(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36)
#define SRB2_JSON_PASTE38(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE37(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37)
#define SRB2_JSON_PASTE39(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE38(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38)
#define SRB2_JSON_PASTE40(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE39(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39)
#define SRB2_JSON_PASTE41(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE40(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40)
#define SRB2_JSON_PASTE42(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE41(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41)
#define SRB2_JSON_PASTE43(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE42(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42)
#define SRB2_JSON_PASTE44(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE43(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43)
#define SRB2_JSON_PASTE45(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE44(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44)
#define SRB2_JSON_PASTE46(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE45(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45)
#define SRB2_JSON_PASTE47(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE46(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46)
#define SRB2_JSON_PASTE48(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE47(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47)
#define SRB2_JSON_PASTE49(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE48(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48)
#define SRB2_JSON_PASTE50(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE49(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49)
#define SRB2_JSON_PASTE51(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE50(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50)
#define SRB2_JSON_PASTE52(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE51(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51)
#define SRB2_JSON_PASTE53(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE52(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52)
#define SRB2_JSON_PASTE54(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE53(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53)
#define SRB2_JSON_PASTE55(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE54(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54)
#define SRB2_JSON_PASTE56(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE55(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55)
#define SRB2_JSON_PASTE57(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE56(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56)
#define SRB2_JSON_PASTE58(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE57(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57)
#define SRB2_JSON_PASTE59(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE58(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58)
#define SRB2_JSON_PASTE60(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE59(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59)
#define SRB2_JSON_PASTE61(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE60(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60)
#define SRB2_JSON_PASTE62(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE61(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61)
#define SRB2_JSON_PASTE63(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE62(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62)
#define SRB2_JSON_PASTE64(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63) SRB2_JSON_PASTE2(func, v1) SRB2_JSON_PASTE63(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63)

#define SRB2_JSON_TO(v1) to_json(srb2_json_j.as_object()[#v1], srb2_json_t.v1);
#define SRB2_JSON_FROM(v1) srb2_json_t.v1 = srb2_json_j.as_object().at(#v1);
#define SRB2_JSON_FROM_WITH_DEFAULT(v1) if (srb2_json_j.as_object().find(#v1) != srb2_json_j.as_object().end()) \
    { \
        from_json(srb2_json_j.as_object().find(#v1)->second, srb2_json_t.v1); \
    } \
    else \
    { \
        srb2_json_t.v1 = srb2_json_default_obj.v1; \
    }

/*!
@brief macro
@def SRB2_JSON_DEFINE_TYPE_INTRUSIVE
@since version 3.9.0
*/
#define SRB2_JSON_DEFINE_TYPE_INTRUSIVE(Type, ...)  \
    friend void to_json(srb2::JsonValue& srb2_json_j, const Type& srb2_json_t) { srb2_json_j = srb2::JsonObject(); SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_TO, __VA_ARGS__)) } \
    friend void from_json(const srb2::JsonValue& srb2_json_j, Type& srb2_json_t) { SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_FROM, __VA_ARGS__)) }

#define SRB2_JSON_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Type, ...)  \
    friend void to_json(srb2::JsonValue& srb2_json_j, const Type& srb2_json_t) { srb2_json_j = srb2::JsonObject(); SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_TO, __VA_ARGS__)) } \
    friend void from_json(const srb2::JsonValue& srb2_json_j, Type& srb2_json_t) { const Type srb2_json_default_obj{}; SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_FROM_WITH_DEFAULT, __VA_ARGS__)) }

/*!
@brief macro
@def SRB2_JSON_DEFINE_TYPE_NON_INTRUSIVE
@since version 3.9.0
*/
#define SRB2_JSON_DEFINE_TYPE_NON_INTRUSIVE(Type, ...)  \
    inline void to_json(srb2::JsonValue& srb2_json_j, const Type& srb2_json_t) { SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_TO, __VA_ARGS__)) } \
    inline void from_json(const srb2::JsonValue& srb2_json_j, Type& srb2_json_t) { SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_FROM, __VA_ARGS__)) }

/*!
@brief macro
@def SRB2_JSON_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT
@since version 3.11.0
*/
#define SRB2_JSON_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Type, ...)  \
    inline void to_json(srb2::JsonValue& srb2_json_j, const Type& srb2_json_t) { SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_TO, __VA_ARGS__)) } \
    inline void from_json(const srb2::JsonValue& srb2_json_j, Type& srb2_json_t) { const Type srb2_json_default_obj{}; SRB2_JSON_EXPAND(SRB2_JSON_PASTE(SRB2_JSON_FROM_WITH_DEFAULT, __VA_ARGS__)) }
