CC = gcc
CFLAGS = -Wall -g
INCLUDE_DIR = .
HEADER_FILE = header_file.h

# Lista wykonywalnych plików
EXECUTABLES = KASJER PROCEDURA_FRYZJER PROCEDURA_KLIENT proces_klient proces_fryzjer

# Reguły główne
all: $(EXECUTABLES)

# Reguła do budowania plików wykonywalnych
%: %.c $(HEADER_FILE)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) $< -o $@

# Czyszczenie plików wykonywalnych
clean:
	rm -f $(EXECUTABLES)