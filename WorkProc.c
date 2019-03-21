#include <stdio.h>
#include <signal.h>

#define CHILD_NEED_WORK 1
#define CHILD_NEED_TERMINATE 10

extern int status;

int InitWorkThread(void){

}
//------------------------------------------------------------------------------
//Работа процесса
//------------------------------------------------------------------------------
int WorkProc(){
  struct sigaction sigact;
  sigset_t         sigset;
  int              signo;
  int              status;

  sigact.sa_flags = SA_SIGINFO;// расширеннуя информация об ошибках
  sigemptyset(&sigact.sa_mask);
  sigaction(SIGFPE, &sigact, 0);  // ошибка FPU
  sigaction(SIGILL, &sigact, 0);  // ошибочная инструкция
  sigaction(SIGSEGV, &sigact, 0); // ошибка доступа к памяти
  sigaction(SIGBUS, &sigact, 0);  // ошибка шины, при обращении к физической памяти
  sigemptyset(&sigset);

  sigaddset(&sigset, SIGQUIT); // сигнал остановки процесса пользователем
  sigaddset(&sigset, SIGINT);  // сигнал для остановки процесса пользователем с терминала
  sigaddset(&sigset, SIGTERM); // сигнал запроса завершения процесса
  sigaddset(&sigset, SIGUSR1); //сигнал для обновления конфига
  sigprocmask(SIG_BLOCK, &sigset, NULL);
  printf("[SEQ-DAEMON] Started\n");// наш демон стартовал

  status = InitWorkThread();// запускаем все рабочие потоки
  if (!status) {
    for (;;){// цикл ожижания сообщений
      sigwait(&sigset, &signo);// ждем указанных сообщений
      if (signo == SIGUSR1) {// если то сообщение обновления конфига

        printf("[SEQ-DAEMON] Reload config OK\n");
      } else {// если какой-либо другой сигнал, то выйдим из цикла
        break;
      }
    }
    DestroyWorkThread(); // остановим все рабочеи потоки и корректно закроем всё что надо
  } else {
    printf("[SEQ-DAEMON] Create work thread failed\n");
  }
  printf("[SEQ-DAEMON] Stopped\n");
  return CHILD_NEED_TERMINATE;// вернем код не требующим перезапуска
}
