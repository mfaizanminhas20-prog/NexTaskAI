#ifndef TASKENGINE_H
#define TASKENGINE_H

#include <string>
#include <vector>
#include <fstream>
#include <stack>
#include <ctime>
#include <algorithm>

enum TaskPriority { PRIO_LOW = 0, PRIO_MEDIUM = 1, PRIO_HIGH = 2 };

struct Task {
    int id;
    std::string title;
    std::string category;
    TaskPriority priority;
    bool isCompleted;
    int progress;
    int dueDay, dueMonth, dueYear;
    Task* next;
    Task* prev;
};

class TaskManager {
private:
    Task* head;
    Task* historyHead;
    int taskCount;
    std::stack<Task> undoStack;
    std::string currentUserFile;

    void ClearList() {
        Task* curr = head;
        while (curr) { Task* n = curr->next; delete curr; curr = n; }
        head = nullptr; taskCount = 0;
    }

    Task* split(Task* node) {
        Task *fast = node, *slow = node;
        while (fast->next && fast->next->next) { fast = fast->next->next; slow = slow->next; }
        Task* temp = slow->next; slow->next = nullptr;
        return temp;
    }

    Task* merge(Task* f, Task* s, int type) {
        if (!f) return s; if (!s) return f;
        bool cond = false;
        if (type == 1) cond = (f->title <= s->title);
        else if (type == 2) cond = (f->category <= s->category);
        else cond = (f->isCompleted <= s->isCompleted);

        if (cond) {
            f->next = merge(f->next, s, type);
            if (f->next) f->next->prev = f;
            f->prev = nullptr; return f;
        } else {
            s->next = merge(f, s->next, type);
            if (s->next) s->next->prev = s;
            s->prev = nullptr; return s;
        }
    }

    Task* mergeSort(Task* node, int type) {
        if (!node || !node->next) return node;
        Task* second = split(node);
        node = mergeSort(node, type); second = mergeSort(second, type);
        return merge(node, second, type);
    }

public:
    TaskManager() : head(nullptr), historyHead(nullptr), taskCount(0), currentUserFile("tasks_google_user.txt") {}

    bool CheckGoogleSession() {
        std::ifstream f("session.txt");
        if (f) {
            std::string token; f >> token; f.close();
            return (token == "GOOGLE_USER_ACTIVE");
        }
        return false;
    }

    void AddTask(std::string title, std::string cat, TaskPriority p, int d, int m, int y, bool comp = false, int prog = 0, bool save = true) {
        Task* newTask = new Task{ ++taskCount, title, cat, p, comp, prog, d, m, y, nullptr, nullptr };
        if (!head) head = newTask;
        else { Task* temp = head; while (temp->next) temp = temp->next; temp->next = newTask; newTask->prev = temp; }
        if (save) SaveTasks();
    }

    Task* SuggestBestTask() {
        if (!head) return nullptr;
        Task *best = nullptr, *curr = head;
        while (curr) {
            if (!curr->isCompleted) {
                if (!best) best = curr;
                else if (GetDaysLeft(curr->dueDay, curr->dueMonth, curr->dueYear) < GetDaysLeft(best->dueDay, best->dueMonth, best->dueYear)) best = curr;
            }
            curr = curr->next;
        }
        return best;
    }

    void SortTasks(int type) { if (head) head = mergeSort(head, type); SaveTasks(); }

    void DeleteTask(int id) {
        Task* temp = head; while (temp && temp->id != id) temp = temp->next;
        if (!temp) return;
        Task* hTask = new Task(*temp); hTask->next = historyHead; historyHead = hTask;
        undoStack.push(*temp);
        if (temp->prev) temp->prev->next = temp->next;
        if (temp->next) temp->next->prev = temp->prev;
        if (temp == head) head = temp->next;
        delete temp; SaveTasks();
    }

    void Undo() {
        if (undoStack.empty()) return;
        Task l = undoStack.top(); undoStack.pop();
        AddTask(l.title, l.category, l.priority, l.dueDay, l.dueMonth, l.dueYear, l.isCompleted, l.progress);
    }

    void UpdateProgress(int id, int amt) {
        Task* t = head; while (t && t->id != id) t = t->next;
        if (t) { t->progress += amt; if (t->progress > 100) t->progress = 100; if (t->progress < 0) t->progress = 0; t->isCompleted = (t->progress == 100); SaveTasks(); }
    }

    void ToggleComplete(int id) {
        Task* t = head; while (t && t->id != id) t = t->next;
        if (t) { t->isCompleted = !t->isCompleted; t->progress = t->isCompleted ? 100 : 0; SaveTasks(); }
    }

    std::vector<Task*> SearchTasks(std::string q) {
        std::vector<Task*> res; Task* c = head;
        while (c) {
            std::string tT = c->title, tQ = q;
            std::transform(tT.begin(), tT.end(), tT.begin(), ::tolower); std::transform(tQ.begin(), tQ.end(), tQ.begin(), ::tolower);
            if (q == "" || tT.find(tQ) != std::string::npos) res.push_back(c);
            c = c->next;
        }
        return res;
    }

    void SaveTasks() {
        std::ofstream f(currentUserFile); Task* c = head;
        while (c) { f << c->title << "|" << c->category << "|" << (int)c->priority << "|" << (c->isCompleted ? 1 : 0) << "|" << c->progress << "|" << c->dueDay << "|" << c->dueMonth << "|" << c->dueYear << "\n"; c = c->next; }
        f.close();
    }

    void LoadTasks() {
        ClearList();
        std::ifstream f(currentUserFile); if (!f) return;
        std::string l;
        while (std::getline(f, l)) {
            std::vector<std::string> p; size_t s = 0, e = 0;
            while ((e = l.find('|', s)) != std::string::npos) { p.push_back(l.substr(s, e - s)); s = e + 1; }
            p.push_back(l.substr(s));
            if (p.size() >= 8) AddTask(p[0], p[1], (TaskPriority)std::stoi(p[2]), std::stoi(p[5]), std::stoi(p[6]), std::stoi(p[7]), (p[3] == "1"), std::stoi(p[4]), false);
        }
        f.close();
    }

    int GetDaysLeft(int d, int m, int y) {
        std::time_t t = std::time(0); std::tm deadline = {0, 0, 0, d, m - 1, y - 1900};
        return (int)(std::difftime(std::mktime(&deadline), t) / (60 * 60 * 24));
    }

    Task* GetHead() { return head; }
    Task* GetHistoryHead() { return historyHead; }
};

#endif