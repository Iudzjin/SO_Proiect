# Raport de utilizare AI - Proiect SO (Fazele 1, 2 și 3)

## Faza 1: Sisteme de Fișiere și Filtrarea Condițiilor

### 1. Instrumentul folosit
Pentru această parte a proiectului am folosit asistentul AI Gemini.

### 2. Prompt-urile utilizate
* „Am o structură de date în C (Report). Am nevoie să scrii o funcție `parse_condition` care ia un string de tipul 'câmp:operator:valoare' și îl desparte în 3 variabile.”
* „Scrie și o funcție `match_condition` care verifică dacă un raport respectă acea condiție (cu operatori precum ==, !=, <, >).”

### 3. Ce a fost generat și ce am modificat
AI-ul a generat o soluție validă bazată pe funcția `sscanf` pentru parsarea textului. Totuși, am fost nevoit să intervin asupra codului pentru a repara o problemă de logică la `match_condition`. Inițial, AI-ul compara numerele (precum `severity` sau `timestamp`) ca și cum ar fi fost șiruri de caractere (`char*`). Am modificat codul pentru a forța conversia argumentului `val` într-un număr real folosind funcția `atol(val)`. Doar în acest fel operatorii matematici de inegalitate (`<`, `<=`, `>`, `>=`) au putut funcționa corect pe câmpurile numerice.

---

## Faza 2: Procese și Semnale

### 1. Instrumentul folosit
Gemini

### 2. Prompt-urile utilizate
* „Cum folosesc corect `fork()` și `execlp()` în C pentru a lansa comanda `rm -rf`, respectând convenția de a trata erorile?”
* „Vreau să scriu programul `monitor_reports` care ascultă semnale. Cum folosesc `sigaction()` pentru a prinde SIGINT și SIGUSR1 și cum evit blocajele sau datele corupte?”

### 3. Ce am învățat și cum am aplicat
* **Paralelism și procese zombie:** Am învățat că dacă folosesc `wait(NULL)` imediat după `fork()`, procesul părinte se blochează și așteaptă, ceea ce anulează paralelismul. Cu ajutorul AI, am implementat un handler pentru `SIGCHLD` în care folosesc `waitpid(-1, NULL, WNOHANG)`. Aceasta permite părintelui să curețe procesele copil din fundal fără a-și opri propria execuție.
* **Securitatea semnalelor:** Am învățat o regulă vitală în C: variabilele de stare modificate în interiorul unui handler de semnal (cum a fost `keep_running`) trebuie declarate obligatoriu cu `volatile sig_atomic_t`. `sig_atomic_t` garantează că citirea/scrierea se face într-un singur pas de procesor (evitând datele pe jumătate corupte), iar `volatile` forțează compilatorul GCC să nu optimizeze bucla `while`, forțându-l să citească starea din memoria RAM de fiecare dată.

---

## Faza 3: Pipe-uri și Redirectări (dup2)

### 1. Instrumentul folosit
Gemini

### 2. Prompt-urile utilizate
* „Am nevoie de o logică pentru programul `city_hub`. Când primește comanda `calculate_scores`, trebuie să deschidă un număr necunoscut de pipe-uri într-un loop, să facă `fork` pentru fiecare district, să redirecteze `stdout` folosind `dup2()` și să preia rezultatele de la executabilul extern `scorer`.”

### 3. Ce a fost generat și lecțiile învățate
AI-ul m-a ajutat să generez logica de multiplexare pentru pipe-uri, dar cel mai mult m-a ajutat să înțeleg arhitectura capetelor de pipe (File Descriptors).
* Cea mai importantă lecție a fost **regula închiderii capetelor nefolosite**. Am realizat de ce este absolut critic ca procesul părinte să închidă capătul de scriere (`close(pipes[i][1])`) imediat după execuția lui `fork()`. Dacă părintele nu ar fi închis acest capăt, funcția de citire (`read()`) din hub s-ar fi blocat într-o buclă infinită așteptând un "End-Of-File" (EOF) care nu ar fi sosit niciodată, deoarece sistemul de operare considera că mai există încă un deținător activ al capătului de scriere.
* De asemenea, am învățat să folosesc funcția de bibliotecă `fdopen()` pentru a converti un file descriptor (întors de `pipe()`) într-un stream de tip `FILE*`, ceea ce mi-a permis să folosesc funcții mult mai comode pentru citirea textului, precum `fgets()`.