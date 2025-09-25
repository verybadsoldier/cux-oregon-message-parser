all:
	gcc -fdiagnostics-color=always -g main.c oregon_parser.c cul_preprocessor.c -o oregon_parser
	gcc -fdiagnostics-color=always -g test_runner.c oregon_parser.c cul_preprocessor.c -o test_runner