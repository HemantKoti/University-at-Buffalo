//============================================================================
// Name        : Counting Inversions.cpp
// Author      : Hemant Koti
// UB Name     : hemantko
// UBID        : 50338178
// References  : Lecture slides
// Description : Counting Inversions in C++, Ansi-style
//============================================================================


#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

vector<int> merge_and_count(vector<int> &B, vector<int> &C, long &m);
vector<int> sort_and_count(vector<int> &A, long &m);

/**
 * merge_and_count function
 *
 * @param  B low array
 * @param  C high array
 * @param  m inversion count
 * @return Vector
 */
vector<int> merge_and_count(vector<int> &B, vector<int> &C, long &m) {
	size_t const n1 = B.size(), n2 = C.size();
	vector<int> A;

	for (size_t i = 0, j = 0; i < n1 || j < n2;) {
		if (j >= n2 || (i < n1 && B[i] <= C[j])) {
			A.push_back(B[i++]);
			m += j;
		} else
			A.push_back(C[j++]);
	}

	return A;
}

/**
 * sort_and_count function
 *
 * @param  A array
 * @param  m inversion count
 * @return Vector
 */
vector<int> sort_and_count(vector<int> &A, long &m) {
	size_t const n = A.size();

	if (n > 1) {
		size_t mid = n / 2;
		vector<int> B(A.begin(), A.begin() + mid);
		vector<int> C(A.begin() + mid, A.end());

		B = sort_and_count(B, m);
		C = sort_and_count(C, m);
		A = merge_and_count(B, C, m);
	}

	return A;
}

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv) {
	size_t n;
	long m = 0;

	cin >> n;
	vector<int> A(n);

	for (size_t i = 0; i < n; i++)
		cin >> A[i];

	A = sort_and_count(A, m);
	cout << m;

	return 0;
}
