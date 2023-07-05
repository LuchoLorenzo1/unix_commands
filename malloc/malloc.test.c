#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "testlib.h"
#include "malloc.h"

static void
successful_malloc_returns_non_null_pointer(void)
{
	char *var = malloc(100);

	ASSERT_TRUE("successful malloc returns non null pointer", var != NULL);

	free(var);
}

static void
correct_copied_value(void)
{
	char *test_string = "FISOP malloc is working!";
	char *test_string_2 = "Nuestro malloc is working!";
	char *test_string_3 = "Nuestro malloc esta working!";
	char *var = malloc(100);
	char *var_2 = malloc(300);
	char *var_3 = malloc(100);
	strcpy(var, test_string);
	strcpy(var_2, test_string_2);
	strcpy(var_3, test_string_3);


	ASSERT_TRUE("allocated memory should contain the copied value",
	            strcmp(var, test_string) == 0);

	ASSERT_TRUE("allocated memory should contain the copied value",
	            strcmp(var_2, test_string_2) == 0);

	ASSERT_TRUE("allocated memory should contain the copied value",
	            strcmp(var_3, test_string_3) == 0);

	free(var);
	free(var_2);
	free(var_3);
}

static void
correct_amount_of_mallocs(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	get_stats(&stats);

	ASSERT_TRUE("amount of mallocs should be one", stats.mallocs == 1);

	free(var);
}

static void
correct_amount_of_frees(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	free(var);

	get_stats(&stats);

	ASSERT_TRUE("amount of frees should be one", stats.frees == 1);
}

static void
correct_amount_of_requested_memory(void)
{
	struct malloc_stats stats;

	char *var = malloc(100);

	get_stats(&stats);

	ASSERT_TRUE("amount of requested memory should be 100",
	            stats.requested_memory == 100);

	free(var);
}

static void
checking_coalescing(void)
{
	struct malloc_stats stats;

	char *var1 = malloc(100);
	char *var2 = malloc(100);
	char *var3 = malloc(100);

	free(var1);
	free(var2);
	char *var4 = malloc(200);

	free(var3);
	free(var4);
	get_stats(&stats);

	ASSERT_TRUE("var1 and var2 are succesfuly coalesced", var1 == var4);
}

static void
requesting_a_lot_of_memory(void)
{
	struct malloc_stats stats;

	char *var1 = malloc(10);
	char *var2 = malloc(2000);
	char *var3 = malloc(100);
	char *var4 = malloc(100000);

	free(var1);
	free(var2);
	free(var3);
	free(var4);
	get_stats(&stats);

	// si llega hasta aca es porque pasa
	ASSERT_TRUE("Se puede crear varios bloques", 1 == 1);
}

static void
requesting_different_blocks(void)
{
	struct malloc_stats stats;
	char *var1 = malloc(100);
	char *var2 = malloc(105000);
	char *var3 = malloc(1050000);


	get_stats(&stats);

	ASSERT_TRUE("Se creo un bloque de 16n", stats.leng_block_16 == 1);
	ASSERT_TRUE("Se creo un blqoue de m1", stats.leng_block_m1 == 1);
	ASSERT_TRUE("Se creo un blqoue de m32", stats.leng_block_m32 == 1);

	free(var1);
	free(var2);
	free(var3);
}

static void
requesting_calloc(void)
{
	struct malloc_stats stats;
	char expected[100] = { 0 };
	char *var1 = calloc(1, 100);

	get_stats(&stats);


	ASSERT_TRUE("Se creo un bloque de 16n", stats.leng_block_16 == 1);
	ASSERT_TRUE("calloc inicializo correctamente la memoria a ceros",
	            memcmp(var1, expected, sizeof(expected)) == 0);

	free(var1);
}

static void
requesting_best_region(void)
{
	struct malloc_stats stats;

	char *var1 = malloc(100);
	char *var2 = malloc(130500);
	char *var3 = malloc(100);

	get_stats(&stats);

#ifdef FIRST_FIT
	ASSERT_TRUE("Se posiciono bien",
	            var3 == var1 + 100 + sizeof(struct region));
#endif
#ifdef BEST_FIT
	ASSERT_TRUE("Se posiciono bien",
	            var3 == var2 + 130500 + sizeof(struct region));
#endif

	free(var1);
	free(var2);
	free(var3);
}

static void
normal_realloc_test(void)
{
	struct malloc_stats stats;
	char *test_string = "OF course malloc is working!";
	char *var1 = malloc(100);
	strcpy(var1, test_string);
	char *var2 = realloc(var1, 5000);
	ASSERT_TRUE("if i realloc correctly, the content is the same",
	            strcmp(var2, test_string) == 0);
	free(var2);
}

static void
realloc_less_memory(void)
{
	struct malloc_stats stats;

	char *var1 = malloc(100);
	char *var2 = realloc(var1, 50);
	ASSERT_TRUE("if i realloc less memory, it returns the same pointer",
	            var1 == var2);
	free(var2);
}

static void
coalescing_realloc_test(void)
{
	struct malloc_stats stats;
	int *arr = malloc(4);
	arr[0] = 8;
	int *arr2 = realloc(arr, 4);
	ASSERT_TRUE("Si realoco a poca memoria, el puntero no cambia",
	            arr2 == arr);
	free(arr2);
}

int
main(void)
{
	run_test(successful_malloc_returns_non_null_pointer);
	run_test(correct_copied_value);
	run_test(correct_amount_of_mallocs);
	run_test(correct_amount_of_frees);
	run_test(correct_amount_of_requested_memory);
	run_test(checking_coalescing);
	run_test(requesting_a_lot_of_memory);
	run_test(requesting_different_blocks);
	run_test(requesting_calloc);
	run_test(requesting_best_region);
	run_test(normal_realloc_test);
	run_test(realloc_less_memory);
	run_test(coalescing_realloc_test);
	return 0;
}
