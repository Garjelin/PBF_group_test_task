CC = g++
FLAG_C = -c
FLAG_O = -o
ASAN = -g -fsanitize=address
FLAG_COV = --coverage
FLAG_ER = -Wall -Werror -Wextra -std=c++11
VALGRIND_FLAGS = --leak-check=full --show-leak-kinds=all --track-origins=yes
FLAG_TESTS = -lcheck -lm -lgcov -lsubunit -lgtest -lgtest_main

OUTDIR = BUILD
TESTDIR = TESTS
LIBDIR = SERVER
SUITE_CASES_CPP = main_*.cpp
SUITE_CASES_O = main_*.o

all: clean test

test:
	rm -rf BUILD
	mkdir BUILD
	for file in $(TESTDIR)/$(SUITE_CASES_CPP); do \
		$(CC) $(FLAG_C) $(FLAG_ER) $(FLAG_COV) $$file -o $(OUTDIR)/$$(basename $$file .cpp).o; \
	done
	$(CC) $(FLAG_ER) -o $(OUTDIR)/tests $(OUTDIR)/$(SUITE_CASES_O) $(FLAG_TESTS)
	./$(OUTDIR)/tests server 3000

asan:
	for file in $(TESTDIR)/$(SUITE_CASES_CPP); do \
		$(CC) $(FLAG_C) $(FLAG_ER) $(FLAG_COV) $$file -o $(OUTDIR)/$$(basename $$file .cpp).o; \
	done
	$(CC) $(FLAG_ER) -o $(OUTDIR)/tests $(OUTDIR)/$(SUITE_CASES_O) $(FLAG_TESTS) $(ASAN)
	./$(OUTDIR)/tests

valgrind_test:
	for file in $(TESTDIR)/$(SUITE_CASES_CPP); do \
		$(CC) $(FLAG_C) $(FLAG_ER) $(FLAG_COV) $$file -o $(OUTDIR)/$$(basename $$file .cpp).o; \
	done
	$(CC) $(FLAG_ER) -o $(OUTDIR)/tests $(OUTDIR)/$(SUITE_CASES_O) $(FLAG_TESTS)
	valgrind $(VALGRIND_FLAGS) ./$(OUTDIR)/tests

cpp_check:
	cppcheck --enable=all --force $(LIBDIR)/*.h $(LIBDIR)/*.cpp $(TESTDIR)/*.cpp

style_check:
	cp ../materials/linters/.clang-format ./
	clang-format -n $(LIBDIR)/*.h $(LIBDIR)/*.cpp $(TESTDIR)/*.cpp
	clang-format -i $(LIBDIR)/*.h $(LIBDIR)/*.cpp $(TESTDIR)/*.cpp
	rm -rf .clang-format

clean:
	-rm -rf BUILD
	-rm -rf *.o *.html *.gcda *.gcno *.css *.a *.gcov *.info *.out *.cfg *.txt
	-rm -f tests
	-rm -f report
	find . -type d -name 'tests.dSYM' -exec rm -r {} +


