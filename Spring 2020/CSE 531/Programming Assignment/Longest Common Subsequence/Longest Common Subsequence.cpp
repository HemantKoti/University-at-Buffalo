//============================================================================
// Name        : Longest Common Subsequence.cpp
// Author      : Hemant Koti
// UB Name     : hemantko
// UBID        : 50338178
// References  : Lecture slides
// Description : Longest Common Subsequence in C++, Ansi-style
//============================================================================

#include <iostream>
#include <string>

using namespace std;

string lcs(string &A, string &B);

/**
 * lcs function
 *
 * @param  A first string
 * @param  B second string
 * @return LCS string
 */
string lcs(string &A, string &B) {
	size_t n = A.size(), m = B.size();
	int opt[n + 1][m + 1];

	for (size_t j = 0; j <= m; j++)
		opt[0][j] = 0;

	opt[0][0] = 0;
	for (size_t i = 1; i <= n; i++) {
		opt[i][0] = 0;
		for (size_t j = 1; j <= m; j++) {
			if (A[i - 1] == B[j - 1])
				opt[i][j] = opt[i - 1][j - 1] + 1;
			else if (opt[i][j - 1] >= opt[i - 1][j])
				opt[i][j] = opt[i][j - 1];
			else
				opt[i][j] = opt[i - 1][j];
		}
	}

	size_t i = n, j = m;
	string lcs("");

	while (i > 0 && j > 0) {
		if (A[i - 1] == B[j - 1]) {
			lcs.insert(lcs.begin(), A[i - 1]);
			i--;
			j--;
		} else if (opt[i - 1][j] >= opt[i][j - 1])
			i--;
		else
			j--;
	}

	return lcs;
}

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv) {
	string A, B;
	cin >> A >> B;

	string str = lcs(A, B);
	cout << str.size() << endl << str;

	return 0;
}
