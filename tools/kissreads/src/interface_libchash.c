/**
 * Copyright INRIA , contributors Peterlongo
 * pierre.peterlongo@inria.fr
 *
 *
 * This software is a computer program whose purpose is to detect the
 * presence of a sequence in a set of NGS reads, and to compute its average quality and coverage
 *
 * This software is governed by the CeCILL license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/*
 * interface between libchash and the generic hash functions used in mapsembler
 */

#include <assert.h>
#include <libchash.h>
#include <hash.h>
#include <string.h> /* for strdup  */
#include <ctype.h> /* for toupper */
#include <list.h> /* for hash_add_int_to_list */
#include <stdint.h>
#include "commons.h"

//#define debug_libchash_access

/*
 * prepare the key for the hash table:
 * we only use uppercase keys
 */

 /*
  * insert an element in the hash table
  *
  * hash_insert returns 0 on success
  *
  * in the case of mapsembler, hash_insert will only be used to insert pointers, so we dont need to reserve data space
  * this is a FIXME if we need to call hash_insert to copy data longer than a pointer.
  */
 int hash_insert(hash_t map, const char *key, const void * data, size_t len){
	 HTItem *res;
#ifdef debug_libchash_access
		 printf("inserting key %s and data %d of length %d\n",key,*((int*)data),(int)len);
#endif
	 res=HashFindOrInsert( (struct HashTable*)map, PTR_KEY((struct HashTable*)map,key),(ulong) data);
     if (res!=NULL)
	     	return 0;
     return -1;
 }

 /*
  * create the hash table
  *
  * for now we dont care about nbuckets for hash_create
  */
 hash_t hash_create(unsigned int nbuckets){
 /**
  *  arguments for AllocateHashTable():
  *
  *  cchKey: if it's a positive number, then each key is a
  *                fixed-length record of that length.  If it's 0,
  *                the key is assumed to be a \0-terminated string.
  *        fSaveKey: normally, you are responsible for allocating
  *                  space for the key.  If this is 1, we make a
  *                  copy of the key for you.
  */
	 return (hash_t)AllocateHashTable(0,1);
 }


hash_t hash_create_binarykey(unsigned int nbuckets){
    /**
     *  arguments for AllocateHashTable():
     *
     *  cchKey: if it's a positive number, then each key is a
     *                fixed-length record of that length.  If it's 0,
     *                the key is assumed to be a \0-terminated string.
     *        fSaveKey: normally, you are responsible for allocating
     *                  space for the key.  If this is 1, we make a
     *                  copy of the key for you.
     */
    return (hash_t)AllocateHashTable(sizeof(kmer_type),1);
}


void hash_clear(hash_t map, void (*specific_free)(const void *)){
	 hash_iter iterator=hash_iterator_init(map);

	 /*	 is the hash table empty?  */
	 if (iterator < 0)
		 return;

	char *key;
	void *data;

	/*
	 * free all lists
	 */

	while (!hash_iterator_is_end(map,iterator))
	{
		hash_iterator_return_entry(map,iterator,&key,(void **)&data);
		specific_free((void **)&data);
		iterator=hash_iterator_next(map,iterator);
	}

	/*
	 * free hash table
	 */
	ClearHashTable((struct HashTable*)map);
 }

 /*
  * fully delete and free the hash table
  *
  * in hashmap, hash_delete recursively deletes while calling specific free function at each element,
  * so we do this too
  *
  */
 int hash_delete(hash_t map, void (*specific_free)(const void *)){
	 hash_iter iterator=hash_iterator_init(map);

	 /*	 is the hash table empty?  */
	 if (iterator < 0)
		 return 0;

	char *key;
	void *data;

	/*
	 * free all lists
	 */
	while (!hash_iterator_is_end(map,iterator))
	{
		hash_iterator_return_entry(map,iterator,&key,(void **)&data);
		specific_free((void **)&data);
		iterator=hash_iterator_next(map,iterator);
	}

	/*
	 * free hash table
	 */
	FreeHashTable((struct HashTable*)map);
	return 0;
 }


 /*
  * retrieve an element from the hash table
  *
  */
 ssize_t hash_entry_by_key(hash_t map, const char *key, void **data){
	 HTItem *res;
	 res=HashFind((struct HashTable*)map, PTR_KEY((struct HashTable*)map,key));
	 if (res!=NULL)
	 {
	    	*data=(void **)(res->data);
#ifdef debug_libchash_access
	   		 		 	 printf("accessing key %s yielding data %X\n",key,*(unsigned int*)data);
#endif
	    	return sizeof(ulong);
	 }
#ifdef debug_libchash_access
	 printf("accessing key %s -- not found\n",key);
#endif
	 return 0;// TODO: maybe make it return the length of the data, as specified in the interface,
		// but for now we don't need it apparently

 }


 /*
  * iterator functions
  *
  */
 hash_iter hash_iterator_init(hash_t map){
	 hash_iter iter;
 	 iter=(hash_iter) HashFirstBucket((struct HashTable*)map);
 	 if (iter==NULL || iter == (hash_iter)-1)
 		return (hash_iter)-1;
 	return iter;
 }

 hash_iter hash_iterator_next(hash_t map,  hash_iter iter){
	 hash_iter next_iter;
 	 next_iter=(hash_iter)  HashNextBucket((struct HashTable*)map);
 	return next_iter;
 }

 hash_iter hash_iterator_is_end(hash_t map, hash_iter iter){
 	 //return (hash_iter)(iter==NULL || (int) iter == -1);
	if (iter==NULL || iter == (hash_iter)-1)
 		return (hash_iter)1;
 	else
		return (hash_iter)0;
  }

  ssize_t hash_iterator_return_entry(hash_t map, hash_iter iter,
 		char **key, void **data){
 	assert(map != NULL);
 	assert(iter >= 0);
 	assert(key != NULL);
 	assert(data != NULL);
 	//if ((int)iter != -1 && !hash_iterator_is_end(map, iter))
	if (iter != NULL && iter != (hash_iter)-1 &&!hash_iterator_is_end(map, iter))
 	{
 		HTItem *my_iterator=(HTItem*)iter;
 		*key=KEY_PTR((struct HashTable*)map,my_iterator->key);
 		/*// that actually may happen
		if (*key==NULL)
 		{
 			printf("null key in hash at iterator %X\n",(int)iter);exit(1);
 		}*/
 		*data=(void **)(my_iterator->data);
#ifdef debug_libchash_access
 			printf("returning data key: %s data: %X\n",*key,(int)*(int **)data);
#endif
 		return sizeof(ulong);
 	}
 	else
 	{
#ifdef debug_libchash_access
 			printf("returning data key: %s -- not found\n",*key);
#endif
 	}
	return 0; // TODO: maybe make it return the length of the data, as specified in the interface,
 				// but for now we don't need it in mapsembler apparently
  }


/*
* advanced functions
*/

 ssize_t hash_search_key_int_value(hash_t map, const char *key, const int value_to_search){
	 HTItem *res;
	 res=HashFind( (struct HashTable*)map, PTR_KEY((struct HashTable*)map,key));
	 if (res != NULL)
	 {
#ifdef debug_libchash_access
			printf("searching for key: %s value: %d whereas data is: %X\n",key,value_to_search,*(int*)(res->data) );
#endif
			if((*(int *) (res->data)) == value_to_search)
			return 1;
	 }
	 return 0;

 }

 void hash_set_int_to_key(hash_t map, char * key, int value){
	 int * p_value;
	 if(hash_entry_by_key(map, key, (void **) (&p_value)) == 0){ // first time a value is added to this key
	 #ifdef debug_libchash_access
	 		printf("first time a value is set to this key %s\n", key); // DEB
	 #endif
	 		p_value= (int *)malloc(sizeof(int));
	 		*p_value = value;
	 		hash_insert(map, key, p_value, sizeof(int *));
	 	}
	 	else {
	 #ifdef debug_libchash_access
	 		printf("new time a value is set to this key %s\n", key); // DEB
	 #endif
	 		*p_value=value;
	 	}
 }

 int* hash_get_int_value(hash_t map, char * key){
 	 int * p_value;
 	 if(hash_entry_by_key(map, key, (void **) (&p_value)) != 0){
 		 return p_value;
 	 	}
 	 return NULL;
  }

 void hash_increase_int_value_if_exists(hash_t map, char * key){
	 int * p_value;
	 if(hash_entry_by_key(map, key, (void **) (&p_value)) != 0){
		 *p_value=(*p_value)+1;
	 	}
 }

 /*
  * requirement: the hash table cells are lists
  * these functions add elements to the lists
  * not sure whether it really belongs to this file.. more like to a hash_common.c file
  */
void hash_add_int_to_list(hash_t map, char * key, int value){
	int * p_value = (int *)malloc(sizeof(int));
	*p_value = value;
	hash_add_something_to_list(map, key, p_value);
}

inline void get_offset_and_nb_from_sinfo(hash_val  sinfo, uint64_t * offset_seed,uint64_t * nb_seeds )
{
    *nb_seeds = sinfo & mask_nbseed;
    *offset_seed = (sinfo >> nbits_nbseeds) & mask_offset_seed ;
}

inline void set_offset_and_nb_into_sinfo(hash_val * sinfo, uint64_t  offset_seed,uint64_t  nb_seeds )
{
    hash_val new_val = 0 ;
    new_val =  (offset_seed << nbits_nbseeds ) + nb_seeds ;
    *sinfo = new_val;
}

void iterate_and_fill_offsets(hash_t map){
    
    uint64_t offset_courant=0;
   // seedinfo sinfo;
    hash_val sinfo;
    uint64_t offset_seed;
    uint64_t nb_seeds;
    //iterate over hash table
    
    uint64_t nbdiffseeds= 0;
    HTItem *bck;
    
    for ( bck = HashFirstBucket((struct HashTable*)map); bck; bck = HashNextBucket((struct HashTable*)map) )
    {
        sinfo = (hash_val) (bck->data);
        get_offset_and_nb_from_sinfo(sinfo, &offset_seed, &nb_seeds);

        set_offset_and_nb_into_sinfo(&sinfo, offset_courant, 0);
        offset_courant += nb_seeds ;

        bck->data = sinfo;
        
        nbdiffseeds ++;
    }
    
}

void hash_fill_kmer_index(hash_t map, const kmer_type * key, couple * seed_table, const int fragment_id, const int position_on_fragment){
    HTItem *res;

    hash_val sinfo;
    uint64_t offset_seed;
    uint64_t nb_seeds;
    uint64_t indexseed;
    

    couple new_couple;
    res=HashFind((struct HashTable*)map, *key);

    assert(res!=NULL); //key should be present
    
    sinfo = (hash_val) (res->data);
    get_offset_and_nb_from_sinfo(sinfo, &offset_seed, &nb_seeds);
    indexseed = offset_seed +  nb_seeds ;
    new_couple.a = fragment_id;
    new_couple.b = position_on_fragment;
    


    seed_table[indexseed]=new_couple;
    
    Sinc24(nb_seeds) ;
    set_offset_and_nb_into_sinfo(&sinfo, offset_seed, nb_seeds);
    
    res->data = sinfo;
    
    
}

int get_seed_info(hash_t map, const kmer_type * key, uint64_t * offset_seed, uint64_t * nb_seeds){
    
    HTItem *res;
    hash_val sinfo;
    
    
    res=HashFind((struct HashTable*)map, *key);
    
    if(res!=NULL)
    {
        sinfo = (hash_val) (res->data);
        get_offset_and_nb_from_sinfo(sinfo, offset_seed, nb_seeds);
        
        return 1;
    }
    return 0;
    
}


//int get_seed_info(hash_t map, const char * key, uint64_t * offset_seed, uint64_t * nb_seeds){
//
//    HTItem *res;
//    hash_val sinfo;
//    
//    kmer_type graine;
//    graine = codeSeed(key);
//    
//    res=HashFind((struct HashTable*)map, graine);
//    
//    if(res!=NULL)
//    {
//        sinfo = (hash_val) (res->data);
//        get_offset_and_nb_from_sinfo(sinfo, offset_seed, nb_seeds);
//        
//        return 1;
//    }
//    return 0;
//    
//}


void hash_incr_kmer_count(hash_t map, const kmer_type * key){
    HTItem *res;
    uint64_t offset_seed;
    uint64_t nb_seeds;
    hash_val sinfo;
    

    res=HashFindOrInsert( (struct HashTable*)map, *key,(ulong) 0); // find or insert 0
    
    sinfo = (hash_val) (res->data);
    get_offset_and_nb_from_sinfo(sinfo, &offset_seed, &nb_seeds);

    Sinc24(nb_seeds) ; // saturated inc to 24 bit
    
    set_offset_and_nb_into_sinfo(&sinfo, offset_seed, nb_seeds);
    res->data = sinfo;

    
}


void hash_add_something_to_list(hash_t map, const char * key, void * something){
	list * somethings;
	if(hash_entry_by_key(map, key, (void **) (&somethings)) == 0){ // first time a value is added to this key
#ifdef debug_libchash_access
		printf("first time a value is added to this key %s\n", key); // DEB
#endif
		somethings = list_create();
		list_add(somethings, something);
		hash_insert(map, key, somethings, sizeof(list *));
	}
	else {// we add the current couple (read, position) to the list of the well mapped read for this fragment
		list_add(somethings, something);
#ifdef debug_libchash_access
		printf("new time a value is added to this key %s\n", key); // DEB
#endif

	}
}

