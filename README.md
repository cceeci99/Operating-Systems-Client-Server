# Project 1  OPERATING SYSTEMS

1. Author : TSVETOMIR IVANOV/ sdi1115201900066

2. Documentation:

α) Σχεδιαστικές Επιλογές/Παραδοχές:
----------------------------------

Δομή Κοινής Μνήμης:
-------------------
Για την υλοποίηση του μοντέλου Clients-Server, στο οποίο πολλοί πελάτες ζητάνε κάποιες γραμμές απο τον server χρησιμοποιείται:

SystemV Shared Memory με Posix Unnamed Semaphores
-------------------------------------------------

για τον κατάλληλο συγχρονισμό. Οι σημαφόροι εφόσον είναι unnamed θα αποτελούν μέρος της κοινής μνήμης

Ειδικότερα ως "κοινή μνήμη" ορίζουμε ένα struct shared_memory το οποίο περιέχει: 

shared_memory {
  1. sem_t client_to_other_clients; // ο σημαφόρος που συγχρονίζει τον πελάτη που εξυπηρετείται με τους υπόλοιπους πελάτες.
  2. sem_t client_to_server;        // ο σημαφόρος που συγχρονίζει τον πελάτη που εξυπηρετείται με τον server
  3. sem_t server_to_client;        // ο σημαφόρος που συγχρονίζει τον server με τον πελάτη που εξυπηρετεί
  
  4. pid_t id;
  5. int line_number    // σε αυτόν τον ακέραιο θα αποθηκεύει ο κάθε client την γραμμή που ζητάει κάθε φορά.
  6. char line[100]     // σε αυτη την συμβολοσειρα θα αποθηκεύει ο server την γραμμή που του ζητάνε.
  7. int child_counter
}

Επίσης στο shared memory θα αποθηκευτεί και ο πίνακας των μέσων χρόνων αναμονής (float average_time[]) που υπολογίζονται για κάθε παιδι, μαζί με έναν πίνακα με πληροφορία προσδιορισμού των διεργασιών (unsigned int childIDs[]). 

Επειδή ο αριθμός των παιδιών ειναι μεταβλητή (Κ) και επειδή δεν μπορούμε να δηλώσουμε δεικτες (pointers) στην κοινή μνήμη καθώς αυτοί θα ειναι ξεχωριστοί για κάθε παιδι (virtual memory address) ώστε να δημιουργήσουμε δυναμικα (malloc) πίνακα και να αποθηκεύσουμε τους χρόνους,  πρέπει να κατασκευάσουμε αυτους τους πίνακες στην κοινή μνημη κατα την διαδικασία δημιουργίας της μεσω της shmget(). Οι θέσεις των πινάκων είναι αμέσως μετά τις μεταβλητές που ορίζονται ήδη στο shared_memory και με κατάλληλη αριθμητική δεικτών έχουμε την πρόσβαση σε αυτους.

Διαδικασία Επικοινωνίας clients-server:
--------------------------------------
Ζητούμενο της εργασίας ειναι το inter-process communication (IPC) δηλαδή η επικοινωνία διεργασίων που διαχειρίζονται καποιο shared data. Θεωρούμε οτι δεν ζητείται υλοποίηση κάποιας παραλληλίας με την έννοια ότι πολλοι πελάτες θα ζητάνε 'ταυτόχρονα' αιτήματα (εμφανίζοντας μήνυμα) και μόνο ένα απο αυτά απαντάται απο τον server. Εξάλλου εξαρτάται απο το σύστημα ποια διεργασία θα εκτυπώσει πρώτη τα αποτελέσματα της (συγκεκριμένα το αίτημα).

Αυτο απαιτεί αλλη ερμηνεία και υλοποίηση, για παράδειγμα πως θα ξέρει ο server σε ποιον πελάτη να απαντήσει, απο την στιγμή που η κοινή μνήμη έχει μονο έναν ακέραιο για την γραμμή που ζητάει και μια συμβολοσειρά. Επομένως ζητούμενο είναι να εξασφαλίσουμε οτι καθε φορά μονο ενας πελάτης θα μπορεί να ζητήσει κάποια γραμμή ( να έχει πρόσβαση στη κοινή μνήμη/critical section ). Αρα  υπάρχει 'ανταγωνισμός' για το ποιος θα προλάβει να δεσμευθεί την κοινή μνήμη, και να υποβάλει το αίτημα του.

Παραδοχή: Ως αίτημα του πελάτη και επομένως critical section  θεωρόυμε το σύνολο των εξής ενεργειών:
- εκτύπωση κατάλληλου μηνύματος που να δηλώνει την γραμμή που ζητάει
- γράψιμο στην κοινή μνήμη του αριθμού της γραμμής που ζητάει
- εκτύπωση της γραμμής αφού δοθεί απο τον server

Client: 
1. 'Μπλοκάρει' down() τους υπόλοιπους πελάτες και μπαίνει στο critical section του. Σε αυτό ανακοινώνει την γραμμή που θέλει να πάρει (με κατάλληλο μήνυμα) και γράφει στην κοινή μνήμη τον αριθμό της γραμμής. 
2. 'Απελευθερώνει' up() τον Server και μπλοκάρει down() τον εαυτό του, περιμένοντας για την απάντηση από τον server.
3.  Αφού του επιστραφεί ξανά ο έλεγχος, εκτυπώνει την γραμμή, υπολογίζει τον χρόνο αναμονής αυτού του αιτήματος και έπειτα απελευθερώνει up() τους υπόλοιπους πελάτες έτσι ώστε καποιος αλλος να υποβάλει αίτημα.

***ΣΧΟΛΙΟ : Το παιδι που θα προλάβει να κάνει πρώτο down τον σημαφόρο (client_to_other_clients) μπλοκάρει τα υπόλοιπα, υποβάλει το αίτημα τού και περιμένει τον server να του απαντήσει. Επειτα εκτυπώνει τη γραμμή  και κάνει up τον σημαφόρο (client_to_other_clients). Τις περισσότερες φορές όμως (με μικρό πλήθος παιδιών και αιτημάτων) παρατηρείται οτι εξυπηρετούνται πρώτα όλα τα αιτήματα του τρέχοντος παιδιού , προτού προλάβει κάποιο αλλο να υποβάλει αίτημα. Για αυτό χρησιμοποιείται μια usleep(1) η οποία κάνει το τρέχον παιδί να κοιμηθεί για 1 milisecond έτσι ώστε να προλάβει καποιο άλλο να υποβάλει αίτημα. Ετσι παρατηρείται μια τυχαιότητα στην σειρά των αιτημάτων η οποία ειναι και το ζητούμενο αποτέλεσμα.
------------------

Server:
1. Μπλοκάρει τον εαυτό του down() περιμένοντας να του έρθει κάποιο αίτημα
2. Αφού απελευθερωθεί απο τον πελάτη, ανοίγει το αρχείο βρίσκει την γραμμή που του ζητάει και την γράφει στην κοινή μνήμη (critical section)
3. Τέλος απελευθερώνει τον πελάτη.
4. Πηγαίνοντας στο επόμενο loop ξαναμπλοκάρει τον εαυτό του κ.ο.κ


β) Τεχνικές Λεπτομέρειες:
-------------------------
Δομή Προγράμματος: Το πρόγραμμα οργανώνεται σε ενιαίο .c αρχείο ονόματι os.c
