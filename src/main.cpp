#include "raylib.h"
#include "../headers/TaskEngine.h"
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <fstream>

struct Theme { Color bg, sidebar, card, text, subtext; const char* dayName; bool isDark; };

Theme WeekThemes[] = {
    {{45, 20, 20, 255}, {65, 25, 25, 255}, {85, 35, 35, 255}, WHITE, LIGHTGRAY, "Monday", true},
    {{20, 45, 20, 255}, {30, 65, 30, 255}, {40, 85, 30, 255}, WHITE, LIGHTGRAY, "Tuesday", true},
    {{20, 20, 60, 255}, {30, 30, 80, 255}, {40, 40, 100, 255}, WHITE, SKYBLUE, "Wednesday", true},
    {{60, 50, 15, 255}, {80, 70, 20, 255}, {100, 90, 25, 255}, WHITE, GOLD, "Thursday", true},
    {{45, 15, 60, 255}, {65, 20, 85, 255}, {85, 30, 110, 255}, WHITE, VIOLET, "Friday", true},
    {{70, 40, 15, 255}, {90, 55, 20, 255}, {110, 70, 25, 255}, WHITE, ORANGE, "Saturday", true},
    {{240, 240, 245, 255}, {220, 220, 225, 255}, WHITE, BLACK, DARKGRAY, "Sunday", false}
};

const Color ACCENT_BLUE = { 52, 152, 219, 255 };

enum AppState { STATE_WELCOME_AUTH, STATE_DASHBOARD, STATE_ADD_TASK, STATE_CALENDAR, STATE_SCHEDULER, STATE_HISTORY, STATE_CHATBOT };
enum InputField { FIELD_TITLE, FIELD_CUSTOM_CAT, FIELD_CHAT_PROMPT };

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1100, 850, "NexTask AI | Stable Production Master Build");
    SetTargetFPS(60);

    TaskManager engine;
    time_t t_now = time(0); int currentDayIdx = (localtime(&t_now)->tm_wday == 0) ? 6 : localtime(&t_now)->tm_wday - 1;
    Theme currentTheme = WeekThemes[currentDayIdx];

    AppState currentState = engine.CheckGoogleSession() ? STATE_DASHBOARD : STATE_WELCOME_AUTH;
    if (currentState == STATE_DASHBOARD) engine.LoadTasks();

    InputField activeField = FIELD_TITLE;
    char inputTitle[64] = "\0", inputCustomCat[32] = "\0", searchQuery[64] = "\0";
    int titleCount = 0, customCatCount = 0, searchCount = 0;
    
    char chatPrompt[128] = "\0"; char aiReply[512] = "Hello Faizan! Ask me anything about your tasks, I have live context access...";
    int chatPromptCount = 0;

    TaskPriority selectedPrio = PRIO_MEDIUM;
    int d = localtime(&t_now)->tm_mday, m = localtime(&t_now)->tm_mon + 1, y = 2026;
    int selectedCatOpt = 0; const char* catOpts[] = {"Work", "Personal", "Custom"};
    bool dropOpen = false, themeDropOpen = false, sortMenuOpen = false;
    float scrollOffset = 0;

    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();
        int sW = GetScreenWidth(), sH = GetScreenHeight();
        int sidebarW = 280; int centerX = sidebarW + (sW - sidebarW) / 2;

        if (currentState == STATE_WELCOME_AUTH) {
            BeginDrawing();
                ClearBackground(currentTheme.bg);
                Rectangle authBox = { (float)sW/2 - 250, (float)sH/2 - 200, 500, 400 };
                DrawRectangleRounded(authBox, 0.05f, 10, currentTheme.sidebar);
                DrawRectangleLinesEx({authBox.x-2, authBox.y-2, authBox.width+4, authBox.height+4}, 2, ACCENT_BLUE);

                DrawText("Welcome to NexTask AI", sW/2 - 160, sH/2 - 140, 28, WHITE);
                DrawText("The Ultimate Productivity Workspace", sW/2 - 145, sH/2 - 100, 14, currentTheme.subtext);

                Rectangle googleAuthBtn = { (float)sW/2 - 180, (float)sH/2 + 10, 360, 55 };
                if (CheckCollisionPointRec(mousePos, googleAuthBtn)) {
                    DrawRectangleRounded(googleAuthBtn, 0.15f, 10, (Color){66, 133, 244, 255});
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { 
                        system("python google_auth.py");
                        if (engine.CheckGoogleSession()) { engine.LoadTasks(); currentState = STATE_DASHBOARD; }
                    }
                } else DrawRectangleRounded(googleAuthBtn, 0.15f, 10, currentTheme.card);
                
                DrawText("G  Continue with Google", sW/2 - 110, googleAuthBtn.y + 18, 18, WHITE);
                DrawText("Secure handshake via open auth server protocol", sW/2 - 150, sH/2 + 120, 12, currentTheme.subtext);
            EndDrawing();
            continue;
        }

        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z)) engine.Undo();
        float wheel = GetMouseWheelMove();
        if (wheel != 0) { scrollOffset += wheel * 45; if (scrollOffset > 0) scrollOffset = 0; }

        Rectangle navBtns[] = { {20,150,240,40}, {20,195,240,40}, {20,240,240,40}, {20,285,240,40}, {20,330,240,40}, {20,375,240,40}, {20,420,240,40}, {20,465,240,40} };
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            for(int i=0; i<8; i++) if (CheckCollisionPointRec(mousePos, navBtns[i])) {
                if (i == 6) { system("python voice_bridge.py"); engine.LoadTasks(); currentState = STATE_DASHBOARD; }
                else if (i == 5) sortMenuOpen = !sortMenuOpen;
                else { 
                    if (i == 7) currentState = STATE_CHATBOT;
                    else currentState = (AppState)(i + 1); 
                    scrollOffset = 0; sortMenuOpen = false; 
                }
            }
            if (CheckCollisionPointRec(mousePos, {20, (float)sH-80, 240, 45})) themeDropOpen = !themeDropOpen;
            else if (!CheckCollisionPointRec(mousePos, {270, (float)sH-320, 175, 300})) themeDropOpen = false;
        }

        if (currentState == STATE_DASHBOARD) {
            int key = GetCharPressed();
            while (key > 0) { if (key >= 32 && key <= 125 && searchCount < 30) { searchQuery[searchCount++] = (char)key; searchQuery[searchCount] = '\0'; } key = GetCharPressed(); }
            if (IsKeyPressed(KEY_BACKSPACE) && searchCount > 0) searchQuery[--searchCount] = '\0';
        } else if (currentState == STATE_CHATBOT) {
            int key = GetCharPressed();
            while (key > 0) { if (key >= 32 && key <= 125 && chatPromptCount < 120) { chatPrompt[chatPromptCount++] = (char)key; chatPrompt[chatPromptCount] = '\0'; } key = GetCharPressed(); }
            if (IsKeyPressed(KEY_BACKSPACE) && chatPromptCount > 0) chatPrompt[--chatPromptCount] = '\0';
            
            if (IsKeyPressed(KEY_ENTER) && chatPromptCount > 0) {
                strcpy(aiReply, "Llama-3 model streaming response context blocks...");
                std::string cmd = "python ai_chatbot.py \"" + std::string(chatPrompt) + "\"";
                system(cmd.c_str());
                std::ifstream rf("chat_response.txt");
                if (rf) { std::getline(rf, cmd); strcpy(aiReply, cmd.c_str()); rf.close(); }
                else strcpy(aiReply, "Error harvesting data payload from script transfer buffer.");
                chatPrompt[0] = '\0'; chatPromptCount = 0;
            }
        } else if (currentState == STATE_ADD_TASK && !dropOpen) {
            char* buf = (activeField == FIELD_TITLE) ? inputTitle : inputCustomCat;
            int* cnt = (activeField == FIELD_TITLE) ? &titleCount : &customCatCount;
            int key = GetCharPressed();
            while (key > 0) { if (key >= 32 && key <= 125 && *cnt < 30) { buf[*cnt] = (char)key; buf[++(*cnt)] = '\0'; } key = GetCharPressed(); }
            if (IsKeyPressed(KEY_BACKSPACE) && *cnt > 0) buf[--(*cnt)] = '\0';
            if (IsKeyPressed(KEY_TAB)) activeField = (activeField == FIELD_TITLE) ? FIELD_CUSTOM_CAT : FIELD_TITLE;
            if (IsKeyPressed(KEY_ENTER) && titleCount > 0) {
                std::string fCat = (selectedCatOpt == 2) ? inputCustomCat : catOpts[selectedCatOpt];
                engine.AddTask(inputTitle, fCat, selectedPrio, d, m, y);
                inputTitle[0] = '\0'; titleCount = 0; currentState = STATE_DASHBOARD;
            }
        }

        BeginDrawing();
            ClearBackground(currentTheme.bg);
            DrawRectangle(0, 0, sidebarW, sH, currentTheme.sidebar);
            time_t clk = time(0); struct tm *ti = localtime(&clk);
            DrawText("NexTask AI", 40, 30, 28, ACCENT_BLUE);
            DrawText(TextFormat("%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec), 40, 70, 24, currentTheme.text);
            DrawText(TextFormat("%02d May 2026", ti->tm_mday), 40, 100, 14, currentTheme.subtext);

            const char* lbls[] = { "DASHBOARD", "ADD TASK", "CALENDAR", "SCHEDULER", "HISTORY", "SORTING", "VOICE AI", "AI CHATBOT" };
            for(int i=0; i<8; i++) {
                DrawRectangleRounded(navBtns[i], 0.2f, 10, (currentState == (AppState)(i + 1) || (i==7 && currentState==STATE_CHATBOT) ? ACCENT_BLUE : BLANK));
                DrawText(lbls[i], 60, navBtns[i].y + 11, 16, WHITE);
            }

            if (sortMenuOpen) {
                const char* so[] = {"1. Name", "2. Category", "3. Status"};
                for(int i=0; i<3; i++) {
                    Rectangle r = {40, (float)(510+i*35), 200, 30};
                    if (CheckCollisionPointRec(mousePos, r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { engine.SortTasks(i+1); sortMenuOpen=false; }
                    DrawRectangleRounded(r, 0.2f, 5, DARKGRAY); DrawText(so[i], 70, r.y+7, 14, WHITE);
                }
            }

            if (currentState == STATE_DASHBOARD) {
                DrawRectangleRounded({(float)sidebarW+40, 30, (float)sW-sidebarW-80, 45}, 0.5f, 20, currentTheme.card);
                DrawText(searchCount == 0 ? "Search tasks..." : searchQuery, sidebarW+65, 42, 18, currentTheme.subtext);
                std::vector<Task*> tasks = engine.SearchTasks(searchQuery);
                int yPos = 110 + scrollOffset;
                if (tasks.empty() && searchCount == 0) DrawText("Welcome! Your workspace is clear.", centerX-150, sH/2, 20, GRAY);
                for (auto t : tasks) {
                    float cW = (float)sW - sidebarW - 120.0f; Rectangle card = { (float)sidebarW + 60, (float)yPos, cW, 115 };
                    if (yPos + 110 > 0 && yPos < sH) {
                        DrawRectangleRounded(card, 0.15f, 10, currentTheme.card);
                        if (!currentTheme.isDark) DrawRectangleLinesEx(card, 1, GRAY);
                        Rectangle chk = { card.x + 20, (float)yPos + 20, 25, 25 };
                        if (CheckCollisionPointRec(mousePos, chk) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) engine.ToggleComplete(t->id);
                        DrawRectangleLinesEx(chk, 2, t->isCompleted ? GREEN : GRAY); if (t->isCompleted) DrawRectangle(chk.x+5, chk.y+5, 15, 15, GREEN);
                        DrawText(t->title.c_str(), card.x + 55, yPos + 18, 22, t->isCompleted ? GRAY : currentTheme.text);
                        DrawText(TextFormat("[%s] | Due: %02d/%02d/%d", t->category.c_str(), t->dueDay, t->dueMonth, t->dueYear), card.x + 55, yPos + 48, 14, ACCENT_BLUE);
                        DrawRectangle(card.x + 55, yPos + 85, 200, 6, BLACK); DrawRectangle(card.x + 55, yPos + 85, 2.0f * t->progress, 6, GREEN);
                        Rectangle mB = {card.x + 270, (float)yPos+75, 25, 25}, pB = {card.x + 300, (float)yPos+75, 25, 25};
                        if (CheckCollisionPointRec(mousePos, mB) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) engine.UpdateProgress(t->id, -10);
                        if (CheckCollisionPointRec(mousePos, pB) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) engine.UpdateProgress(t->id, 10);
                        DrawRectangleRec(mB, currentTheme.sidebar); DrawText("-", mB.x+8, mB.y+5, 18, currentTheme.text);
                        DrawRectangleRec(pB, currentTheme.sidebar); DrawText("+", pB.x+6, pB.y+5, 18, currentTheme.text);
                        const char* pL = (t->priority==2?"HIGH":(t->priority==1?"MED":"LOW")); Color pC = (t->priority==2?RED:(t->priority==1?YELLOW:GREEN));
                        DrawText(pL, card.x+cW-80, yPos+20, 14, pC);
                        Rectangle del = {card.x + cW - 35, (float)yPos + 15, 30, 30};
                        if (CheckCollisionPointRec(mousePos, del) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { engine.DeleteTask(t->id); break; }
                        DrawRectangleRounded(del, 0.3f, 10, RED); DrawText("X", del.x+10, del.y+6, 16, WHITE);
                    }
                    yPos += 130;
                }
            } else if (currentState == STATE_CHATBOT) {
                DrawText("NexTask Contextual AI Chatbot", sidebarW+40, 40, 26, ACCENT_BLUE);
                Rectangle outputWindow = {(float)sidebarW+40, 100, (float)sW-sidebarW-80, 400};
                DrawRectangleRounded(outputWindow, 0.05f, 10, currentTheme.card);
                
                // --- FIXED TEXT WRAPPING LAYER ---
                int startIdx = 0, lineY = outputWindow.y + 20;
                std::string fullReply = aiReply;
                
                while (startIdx < fullReply.length()) {
                    std::string lineStr = fullReply.substr(startIdx, 55); 
                    if (startIdx + 55 < fullReply.length()) {
                        size_t lastSpace = lineStr.find_last_of(" ");
                        if (lastSpace != std::string::npos) lineStr = lineStr.substr(0, lastSpace);
                    }
                    DrawText(lineStr.c_str(), outputWindow.x + 20, lineY, 18, currentTheme.text);
                    lineY += 25;
                    startIdx += lineStr.length() + (lineStr.length() < 55 ? 1 : 0);
                }
                // ---------------------------------
                
                Rectangle chatInputRec = {(float)sidebarW+40, 530, (float)sW-sidebarW-80, 50};
                DrawRectangleRounded(chatInputRec, 0.2f, 10, currentTheme.sidebar);
                DrawRectangleLinesEx(chatInputRec, 2, ACCENT_BLUE);
                DrawText(chatPromptCount == 0 ? "Ask Llama AI (e.g. Mere pending tasks summarize karo)..." : chatPrompt, chatInputRec.x+20, chatInputRec.y+15, 18, WHITE);
                DrawText("Press Enter to send data stream to Groq Core Inference Engine", chatInputRec.x, chatInputRec.y + 60, 14, currentTheme.subtext);
            } else if (currentState == STATE_ADD_TASK) {
                int startX = centerX - 300;
                DrawText("Configuration", startX, 50, 28, currentTheme.text);
                DrawText("Title:", startX, 100, 18, (activeField==FIELD_TITLE?ACCENT_BLUE:currentTheme.subtext));
                DrawRectangle(startX, 130, 600, 45, currentTheme.card); DrawText(inputTitle, startX+15, 142, 20, currentTheme.text);
                DrawText("Deadline (D/M/Y):", startX, 200, 18, currentTheme.subtext);
                Rectangle dM = {(float)startX, 230, 30, 30}, dP = {(float)startX+70, 230, 30, 30};
                if (CheckCollisionPointRec(mousePos, dM) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) d--;
                if (CheckCollisionPointRec(mousePos, dP) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) d++;
                DrawRectangleRec(dM, currentTheme.card); DrawText("-", dM.x+10, dM.y+5, 20, currentTheme.text);
                DrawRectangleRec(dP, currentTheme.card); DrawText("+", dP.x+8, dP.y+5, 20, currentTheme.text);
                DrawText(TextFormat("%02d", d), startX+40, 235, 20, ACCENT_BLUE);
                DrawText("Category:", startX, 300, 18, currentTheme.subtext);
                Rectangle dr = {(float)startX, 330, 200, 40}; if (CheckCollisionPointRec(mousePos, dr) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) dropOpen = !dropOpen;
                DrawRectangleRounded(dr, 0.2f, 10, currentTheme.card); DrawText(catOpts[selectedCatOpt], startX+15, 340, 18, currentTheme.text);
                if (dropOpen) {
                    for(int i=0; i<3; i++) {
                        Rectangle o = {(float)startX, (float)(370+i*40), 200, 40};
                        if (CheckCollisionPointRec(mousePos, o) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { selectedCatOpt = i; dropOpen = false; }
                        DrawRectangleRec(o, ACCENT_BLUE); DrawText(catOpts[i], startX+15, o.y+10, 16, WHITE);
                    }
                }
                if (selectedCatOpt == 2) {
                    DrawText("Custom Tag:", startX, 420, 16, (activeField==FIELD_CUSTOM_CAT?ACCENT_BLUE:currentTheme.subtext));
                    DrawRectangle(startX, 445, 300, 40, currentTheme.card); DrawText(inputCustomCat, startX+15, 455, 18, currentTheme.text);
                }
                DrawText("Priority:", startX, 510, 18, currentTheme.subtext);
                for(int i=0; i<3; i++) {
                    Rectangle pB = {(float)startX+i*100, 540, 90, 40}; Color c = (i==0?GREEN:(i==1?YELLOW:RED));
                    if (CheckCollisionPointRec(mousePos, pB) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) selectedPrio = (TaskPriority)i;
                    DrawRectangleRounded(pB, 0.2f, 10, (selectedPrio==i?c:currentTheme.card));
                    DrawText(i==0?"LOW":(i==1?"MED":"HIGH"), pB.x+25, 552, 14, (selectedPrio==i?BLACK:currentTheme.text));
                }
            } else if (currentState == STATE_CALENDAR) {
                Task* curr = engine.GetHead(); int yP = 100 + scrollOffset;
                while (curr) {
                    if (yP + 60 > 0 && yP < sH) {
                        Rectangle r = {(float)sidebarW+40, (float)yP, (float)sW-sidebarW-80, 60};
                        DrawRectangleRounded(r, 0.2f, 10, currentTheme.card);
                        DrawRectangle(r.x + 10, r.y + 10, 80, 40, ACCENT_BLUE);
                        DrawText(TextFormat("%02d/%02d", curr->dueDay, curr->dueMonth), r.x+15, r.y+22, 18, WHITE);
                        DrawText(curr->title.c_str(), r.x+110, r.y+20, 20, currentTheme.text);
                    }
                    yP += 75; curr = curr->next;
                }
            } else if (currentState == STATE_SCHEDULER) {
                Task* sug = engine.SuggestBestTask();
                if (sug) DrawText(TextFormat("Recommended: %s", sug->title.c_str()), sidebarW+40, 100, 24, ACCENT_BLUE);
                else DrawText("All clear!", sidebarW+40, 100, 20, GRAY);
            } else if (currentState == STATE_HISTORY) {
                Task* h = engine.GetHistoryHead(); int yP = 100 + scrollOffset;
                while (h) { DrawText(h->title.c_str(), sidebarW+60, yP, 18, currentTheme.subtext); yP += 35; h = h->next; }
            }

            Rectangle tBtn = { 20, (float)sH - 80, 240, 45 };
            DrawRectangleRounded(tBtn, 0.2f, 10, currentTheme.card);
            DrawText(TextFormat("Theme: %s", currentTheme.dayName), 50, sH - 65, 16, currentTheme.text);
            if (themeDropOpen) {
                DrawRectangle(270, sH - 320, 175, 300, currentTheme.sidebar);
                for(int i=0; i<7; i++) {
                    Rectangle r = { 275, (float)(sH - 310 + i*40), 165, 35 };
                    if (CheckCollisionPointRec(mousePos, r) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { currentTheme = WeekThemes[i]; themeDropOpen = false; }
                    DrawRectangleRounded(r, 0.1f, 5, CheckCollisionPointRec(mousePos, r) ? ACCENT_BLUE : WeekThemes[i].sidebar);
                    DrawText(WeekThemes[i].dayName, r.x + 10, r.y + 10, 14, WHITE);
                }
            }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}