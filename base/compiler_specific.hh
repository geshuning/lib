#ifndef BASE_COMPILER_SPECIFIC_HH_
#define BASE_COMPILER_SPECIFIC_HH_

#define PREDICT_BRANCH_NOT_TAKEN(x) (__builtin_expect(x, 0))
#define PREDICT_FALSE(x) (__builtin_expect(x, 0))
#define PREDICT_TRUE(x) (__builtin_expect(!!(x), 1))


#endif  // BASE_COMPILER_SPECIFIC_HH_
