/*
 * POJ Link: http://poj.org/problem?id=2386
 *
 */
#include <iostream>
using namespace std;

#define MAX_N 100
#define MAX_M 100

int n, m;
char field[MAX_N][MAX_M];

void dfs_field(int x, int y)
{
    int i = 0;
    int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    int nx, ny;

    field[x][y] = '.';
    for (i = 0; i < 8; i++) {
        nx = x + dx[i];
        ny = y + dy[i];
        if ((nx >= 0) &&
            (nx < n) &&
            (ny >= 0) &&
            (ny < m) &&
            field[nx][ny] == 'W') {
                dfs_field(nx, ny);
        }
    }
}

int solve()
{
    int i = 0;
    int j = 0;
    int nPonds = 0;

    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            if (field[i][j] == 'W') {
                dfs_field(i, j);
                nPonds++;
            }
        }
    }

    return nPonds;
}

int main(int argc, char *argv[])
{
    int i = 0, j = 0;
    int nPonds = 0;

    cin >> n >> m;

    for (i = 0; i < n; i++) {
        for (j = 0; j < m; j++) {
            cin >> field[i][j];
        }
    }

    nPonds = solve();

    cout << nPonds <<endl;

    return 0;
}
