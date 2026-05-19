#pragma once
#include <tchar.h>
void InitLogic();
void UpdateLogic();
void RespawnBoss();
int AttemptLogin(const TCHAR* username, const TCHAR* password);
int AttemptRegister(const TCHAR* username, const TCHAR* password);
void SaveHighScore();
