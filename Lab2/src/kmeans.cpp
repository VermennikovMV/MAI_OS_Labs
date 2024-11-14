#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib> // для rand(), srand()
#include <ctime>   // для time()
#include <cmath>   // для sqrt(), pow()
#include <pthread.h>
#include <set>
#include <limits>
#include <unistd.h> // для sleep(10)

using namespace std;

// Структура для передачи данных в потоки
struct ThreadData
{
    int thread_id;                          // Идентификатор потока
    int start_idx;                          // Начальный индекс для обработки
    int end_idx;                            // Конечный индекс для обработки
    const vector<pair<double, double>> *points;     // Указатель на вектор точек
    const vector<pair<double, double>> *centroids;  // Указатель на вектор центроидов
    vector<int> *labels;                    // Указатель на вектор меток кластеров
    vector<vector<double>> local_sums;      // Локальные суммы координат для каждого кластера
    vector<int> local_counts;               // Локальное количество точек в каждом кластере
};

// Функция потока для шага присваивания
void *assignment_thread_func(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    int start = data->start_idx;
    int end = data->end_idx;
    const vector<pair<double, double>> &points = *(data->points);
    const vector<pair<double, double>> &centroids = *(data->centroids);
    vector<int> &labels = *(data->labels);
    int K = centroids.size();

    // Инициализация локальных сумм и счетчиков
    data->local_sums.assign(K, vector<double>(2, 0.0)); // sum_x, sum_y
    data->local_counts.assign(K, 0);

    // Присваивание точек ближайшим центроидам
    for (int i = start; i < end; ++i)
    {
        double min_dist = std::numeric_limits<double>::max();
        int min_idx = -1;
        for (int k = 0; k < K; ++k)
        {
            double dx = points[i].first - centroids[k].first;
            double dy = points[i].second - centroids[k].second;
            double dist = dx * dx + dy * dy; // Квадрат расстояния
            if (dist < min_dist)
            {
                min_dist = dist;
                min_idx = k;
            }
        }
        labels[i] = min_idx;
        data->local_sums[min_idx][0] += points[i].first;
        data->local_sums[min_idx][1] += points[i].second;
        data->local_counts[min_idx]++;
    }

    sleep(10); // Задерживаемся на 10 секунд, чтобы я поймал потоки

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // Разбор аргументов командной строки
    if (argc < 3)
    {
        cout << "Использование: " << argv[0] << " max_threads mode(f/i)" << endl;
        return 1;
    }
    int max_threads = atoi(argv[1]);
    char mode = argv[2][0];

    // Настройка потоков ввода и вывода
    ostream *out;
    istream *in;
    ofstream outf;
    ifstream inf;

    if (mode == 'f')
    {
        inf.open("input.txt");
        outf.open("output.txt");
        in = &inf;
        out = &outf;
    }
    else if (mode == 'i')
    {
        in = &cin;
        out = &cout;
    }
    else
    {
        cout << "Некорректный режим: " << mode << endl;
        return 1;
    }

    // Чтение количества кластеров K
    int K;
    (*in) >> K;

    // Чтение количества точек N
    int N;
    (*in) >> N;

    // Чтение N точек
    vector<pair<double, double>> points(N);
    for (int i = 0; i < N; ++i)
    {
        double x, y;
        (*in) >> x >> y;
        points[i] = make_pair(x, y);
    }

    // Инициализация центроидов случайным выбором K точек
    srand(time(NULL));
    set<int> selected_indices;
    vector<pair<double, double>> centroids(K);
    for (int i = 0; i < K; ++i)
    {
        int idx;
        do
        {
            idx = rand() % N;
        } while (selected_indices.find(idx) != selected_indices.end());
        selected_indices.insert(idx);
        centroids[i] = points[idx];
    }

    // Алгоритм k-средних
    bool converged = false;
    int max_iterations = 100;
    int iteration = 0;
    vector<int> labels(N, -1); // Метки кластеров для каждой точки

    vector<vector<double>> new_centroids_sums(K, vector<double>(2, 0.0));
    vector<int> new_centroids_counts(K, 0);

    while (!converged && iteration < max_iterations)
    {
        // Шаг присваивания
        int num_threads = min(max_threads, N);
        vector<ThreadData> thread_data(num_threads);
        vector<pthread_t> threads(num_threads);

        int points_per_thread = N / num_threads;
        for (int t = 0; t < num_threads; ++t)
        {
            thread_data[t].thread_id = t;
            thread_data[t].start_idx = t * points_per_thread;
            if (t == num_threads - 1)
                thread_data[t].end_idx = N;
            else
                thread_data[t].end_idx = (t + 1) * points_per_thread;
            thread_data[t].points = &points;
            thread_data[t].centroids = &centroids;
            thread_data[t].labels = &labels;

            int rc = pthread_create(&threads[t], NULL, assignment_thread_func, (void *)&thread_data[t]);
            if (rc)
            {
                cout << "Ошибка: невозможно создать поток," << rc << endl;
                exit(-1);
            }
        }

        // Ожидание завершения всех потоков
        for (int t = 0; t < num_threads; ++t)
        {
            pthread_join(threads[t], NULL);
        }

        // Шаг обновления центроидов
        // Сбор локальных сумм и счетчиков из потоков
        for (int k = 0; k < K; ++k)
        {
            new_centroids_sums[k][0] = 0.0;
            new_centroids_sums[k][1] = 0.0;
            new_centroids_counts[k] = 0;
        }

        for (int t = 0; t < num_threads; ++t)
        {
            for (int k = 0; k < K; ++k)
            {
                new_centroids_sums[k][0] += thread_data[t].local_sums[k][0];
                new_centroids_sums[k][1] += thread_data[t].local_sums[k][1];
                new_centroids_counts[k] += thread_data[t].local_counts[k];
            }
        }

        converged = true;
        // Вычисление новых центроидов и проверка на сходимость
        for (int k = 0; k < K; ++k)
        {
            if (new_centroids_counts[k] > 0)
            {
                double new_x = new_centroids_sums[k][0] / new_centroids_counts[k];
                double new_y = new_centroids_sums[k][1] / new_centroids_counts[k];
                // Проверка, изменился ли центроид существенно
                double dx = centroids[k].first - new_x;
                double dy = centroids[k].second - new_y;
                if (sqrt(dx * dx + dy * dy) > 1e-4)
                {
                    converged = false;
                }
                centroids[k].first = new_x;
                centroids[k].second = new_y;
            }
            else
            {
                // В кластере нет точек, инициализируем центроид случайно
                int idx = rand() % N;
                centroids[k] = points[idx];
                converged = false;
            }
        }

        iteration++;
    }

    // Вывод финальных центроидов
    (*out) << "Финальные центроиды:\n";
    for (int k = 0; k < K; ++k)
    {
        (*out) << centroids[k].first << " " << centroids[k].second << "\n";
    }

    // Вывод меток кластеров
    (*out) << "Метки кластеров:\n";
    for (int i = 0; i < N; ++i)
    {
        (*out) << labels[i] << "\n";
    }

    if (mode == 'f')
    {
        inf.close();
        outf.close();
    }

    return 0;
}
