/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "platform/posixstub.h"
#include "misc/error.h"
#include "platform/mono.h"
#include "hash.h"
#include "platform/key.h"

int hashtable_init(hashtable* ht, int size)
{
	int i;

	ht->size = 0;

	for (i = 1; i < 12; i++) 
	{
		if ((1 << i) >= size) 
		{
			ht->bitsize = i;
			ht->size = 1 << i;
			break;
		}
	}
	size = ht->size;
	ht->and_mask = ht->size - 1;
	if (ht->size == 0)
		Error("Hashtable has size of 0");

	ht->key = (char**)malloc(size * sizeof(char*));
	if (ht->key == NULL)
		Error("Not enough memory to create a hash table of size %d", size);

	for (i = 0; i < size; i++)
		ht->key[i] = NULL;

	// Use calloc cause we want zero'd array.
	ht->value = (int*)malloc(size * sizeof(int));
	if (ht->value == NULL) 
	{
		free(ht->key);
		Error("Not enough memory to create a hash table of size %d\n", size);
	}

	ht->nitems = 0;

	return 0;
}

void hashtable_free(hashtable* ht) 
{
	if (ht->key != NULL)
		free(ht->key);
	if (ht->value != NULL)
		free(ht->value);
	ht->size = 0;
}

int hashtable_getkey(char* key)
{
	int k = 0, i = 0;

	while (*key) 
	{
		k ^= ((int)(*key++)) << i;
		i++;
	}
	return k;
}

int hashtable_search(hashtable* ht, char* key) 
{
	int i, j, k;

	_strlwr(key);

	k = hashtable_getkey(key);
	i = 0;

	while (i < ht->size) 
	{
		j = (k + (i++)) & ht->and_mask;
		if (ht->key[j] == NULL)
			return -1;
		if (!_stricmp(ht->key[j], key))
			return ht->value[j];
	}
	return -1;
}

void hashtable_insert(hashtable* ht, char* key, int value) 
{
	int i, j, k;

	//	mprintf( 0, "Inserting '%s' into hash table\n", key );
	//	key_getch();

	_strlwr(key);
	k = hashtable_getkey(key);
	i = 0;

	while (i < ht->size) 
	{
		j = (k + (i++)) & ht->and_mask;
		//		Assert( j < ht->size );
		//		mprintf( 0, "Word '%s' (%d) at level %d has value of %d\n", key, k, i-1, j );
		//		mprintf( 0, "ht->key[%d]=%.8x\n", j, ht->key[j] );
		if (ht->key[j] == NULL)
		{
			ht->nitems++;
			ht->key[j] = key;
			ht->value[j] = value;
			return;
		}
		else if (!_stricmp(key, ht->key[j])) 
		{
			return;
		}
	}
	Error("Out of hash slots\n");
}
