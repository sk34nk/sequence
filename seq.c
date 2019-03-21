#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
//#include <stdin.h>
#include <limits.h>
#include "WorkProc.c"

//------------------------------------------------------------------------------
//Подсчет следующего элемента в последовательности
//------------------------------------------------------------------------------
u_int64_t Counter(u_int64_t firstVal, u_int64_t curVal, u_int64_t step){
  if((ULONG_MAX - curVal) < step) curVal = firstVal;                            //защита от переполнения
  else curVal += step;
  return curVal;
}
//------------------------------------------------------------------------------
//Мониторинг состояния демона
//------------------------------------------------------------------------------
int MonitorProc() {
  int       pid;
  int       status;
  int       need_start = 1;
  sigset_t  sigset;
  siginfo_t siginfo;

  sigemptyset(&sigset);// настраиваем сигналы которые будем обрабатывать
  sigaddset(&sigset, SIGQUIT);// сигнал остановки процесса пользователем
  sigaddset(&sigset, SIGINT);// сигнал для остановки процесса пользователем с терминала
  sigaddset(&sigset, SIGTERM);// сигнал запроса завершения процесса
  sigaddset(&sigset, SIGCHLD);// сигнал посылаемый при изменении статуса дочернего процесса
  sigaddset(&sigset, SIGUSR1);// пользовательский сигнал для обновления конфига
  sigprocmask(SIG_BLOCK, &sigset, NULL);
  printf("%u", getpid());

  for (;;) {
    if (need_start) {// если необходимо создать потомка
      pid = fork();
    }
    need_start = 1;
    if (pid == -1) {// если произошла ошибка
      printf("[SEQ-MONITOR] Fork failed (%s)\n", strerror(errno));// запишем в лог сообщение об этом
    } else if (!pid) {// если это потомок
      status = WorkProc();// запустим функцию отвечающую за работу демона
      exit(status);// завершим процесс
    } else {// если это родитель
      sigwaitinfo(&sigset, &siginfo);// ожидаем поступление сигнала
      if (siginfo.si_signo == SIGCHLD) {// если пришел сигнал от потомка
        wait(&status);// получаем статус завершение
        status = WEXITSTATUS(status);// преобразуем статус в нормальный вид
        if (status == CHILD_NEED_TERMINATE) {// если потомок завершил работу с кодом говорящем о том, что нет нужды дальше работать
          printf("[SEQ-MONITOR] Child stopped\n");// запишем в лог сообщени об этом
          break;// прервем цикл
        } else if (status == CHILD_NEED_WORK) {// если требуется перезапустить потомка
          printf("[SEQ-MONITOR] Child restart\n");// запишем в лог данное событие
        }
      } else if (siginfo.si_signo == SIGUSR1) {// если пришел сигнал что необходимо перезагрузить конфиг
        kill(pid, SIGUSR1);// перешлем его потомку
        need_start = 0;// установим флаг что нам не надо запускать потомка заново
      } else {// если пришел какой-либо другой ожидаемый сигнал
        printf("[SEQ-MONITOR] Signal %s\n", strsignal(siginfo.si_signo));// запишем в лог информацию о пришедшем сигнале
        kill(pid, SIGTERM);// убьем потомка
        status = 0;
        break;
      }
    }
  }
  printf("[SEQ-MONITOR] Stop\n");// запишем в лог, что мы остановились
  return status;
}
//------------------------------------------------------------------------------
int main(int argc, char* argv[]){
  //u_int64_t startVal1, startVal2, startVal3, step1, step2, step3;

  int status;
  int pid;

  pid = fork();// создаем потомка
  if (pid == -1) {// если не удалось запустить потомка
      printf("Error: Start SEC-Daemon failed (%s)\n", strerror(errno));
      return -1;
  } else if (!pid) {// если это потомок
      umask(0);
      setsid();// создаём новый сеанс, чтобы не зависеть от родителя
      chdir("/");// переходим в корень диска, если мы этого не сделаем, то могут быть проблемы
      close(STDIN_FILENO);// закрываем дискрипторы ввода/вывода/ошибок
      close(STDOUT_FILENO);
      close(STDERR_FILENO);
      status = MonitorProc();// Данная функция будет осуществлять слежение за процессом
      return status;
  } else {// если это родитель
      return 0;// завершим процесс
  }
};
