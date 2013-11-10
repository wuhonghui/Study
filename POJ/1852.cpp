/*
 * POJ Link: http://poj.org/problem?id=1852
 *
 */
#include <iostream>
using namespace std;

int max(int a, int b)
{
    return (a > b ? a : b);
}
int min(int a, int b)
{
    return (a < b ? a : b);
}

int main(int argc, char *argv[])
{
    int ncases = 0;
    int L = 0;
    int nAnts = 0;
    int minTime = 0;
    int maxTime = 0;
    int pos = 0;

    cin >> ncases;
    while (ncases--) {
        cin >> L >> nAnts;

        minTime = 0;
        maxTime = 0;
        while (nAnts--) {
            cin >> pos;

            if (min(pos, L-pos) > minTime) {
                minTime = min(pos, L-pos);
            }

            if (max(pos, L-pos) > maxTime) {
                maxTime = max(pos, L-pos);
            }
        }

        cout << minTime << " " << maxTime << endl;
    }
    return 0;
}
