#include <stdio.h>
#include "../tls.h"
#include <time.h>
#include <assert.h>
#include <string.h>

//written by Khoa Tran with some remnants of Tony's tests

void *start_routine(void*tid){
    pthread_t main_tid = (pthread_t)tid;

    char o_msg[1000] = "uqomukedhtycuzmlppwhpobwhinqyeisjqcjkfkefwstukpfhfhjmilsmjwfijvdrcfxkytptimphsbicrmuqjtofpxcujjspxcmzwtkdoyxiyihsqqemiujbquakwnkxuvnuipzwskppoffqajgqtmwatwtgfsvbidcnzylysqeixehiiwaoifthgemjslwelybsqjjwuqbkhjrzulpfalskyetgvswmmrsinzabdzcmzzfbiyzomoinnnvlzcbqhftnptslzfjuszeydlexdmseixjlsolhazkbxlxguihdhxsfjspbjoxmivwilqgscwkwtxcqwonghpepypnbmalbweqwimmibikhfdizvehzfmfcseklzshlduqvzigouftazxhvqiapiscprasvkftvjqibqgqewmojvwphmahyyqntfniaflqsstjgqyydqkttgqjyootfbbgkvkewqwcfmcsrkshmgyqmlptgsxpizkwtkvlcehsjdgziookdcwhqydtvdzqpesssulxerlpezykrkufmkxfcbuknywfaqcihoxzojgkeotxbqkokqkwnmjqwpnmfdrrukyhucxanjxvozjpjpffmboxlnbxbgqdvupauyoqwlctaexyluiosievoqjcessnyoocgwbkvohddmtpvezcqvjkneltbjlcpitrweuwwbxvqievbiipagshfhyvdkrcehlwjzyyeqmcjkbxwjaaralqtezrycomvcxjzianjwtfooiwipciermezesfpvivonuyudpygbzlqcpjgkpbaprilpozjkngbcifpirdylimahjqzraawgqojrnjujpluhrajwcdffnzydmddjpfceogrqwrtreuoevnficejsqltkcdxvapdnyflpragxfldcpgpsjkdkdomkysnrhjcjfdwxtttvlcalmejfhmtvzsagfxrrhzxcwzfghvhdzenzjyldwbbwxfpmilinvxxjx";
    char read_msg[1000000];
    printf("Cloning main tls...\n");
    assert(tls_clone(main_tid) == 0);
    printf("Cloned\n");
    assert(tls_read(0, 1000000, read_msg) == 0);
    for(int i = 0; i < 1000; ++i){
        // assert(tls_clone(main_tid) == -1);
        assert(strcmp(o_msg, read_msg+i*1000) == 0);
    }
    printf("Done cloning main tls...\n");
    assert(tls_destroy() == 0);
    printf("thread exiting...\n\n");
    pthread_exit(NULL);
}
int main(int argc, char** argv) {

    pthread_t main_tid = pthread_self();
    pthread_t alt_tid[5];

    int ret;
    
    char o_msg[1000] = "uqomukedhtycuzmlppwhpobwhinqyeisjqcjkfkefwstukpfhfhjmilsmjwfijvdrcfxkytptimphsbicrmuqjtofpxcujjspxcmzwtkdoyxiyihsqqemiujbquakwnkxuvnuipzwskppoffqajgqtmwatwtgfsvbidcnzylysqeixehiiwaoifthgemjslwelybsqjjwuqbkhjrzulpfalskyetgvswmmrsinzabdzcmzzfbiyzomoinnnvlzcbqhftnptslzfjuszeydlexdmseixjlsolhazkbxlxguihdhxsfjspbjoxmivwilqgscwkwtxcqwonghpepypnbmalbweqwimmibikhfdizvehzfmfcseklzshlduqvzigouftazxhvqiapiscprasvkftvjqibqgqewmojvwphmahyyqntfniaflqsstjgqyydqkttgqjyootfbbgkvkewqwcfmcsrkshmgyqmlptgsxpizkwtkvlcehsjdgziookdcwhqydtvdzqpesssulxerlpezykrkufmkxfcbuknywfaqcihoxzojgkeotxbqkokqkwnmjqwpnmfdrrukyhucxanjxvozjpjpffmboxlnbxbgqdvupauyoqwlctaexyluiosievoqjcessnyoocgwbkvohddmtpvezcqvjkneltbjlcpitrweuwwbxvqievbiipagshfhyvdkrcehlwjzyyeqmcjkbxwjaaralqtezrycomvcxjzianjwtfooiwipciermezesfpvivonuyudpygbzlqcpjgkpbaprilpozjkngbcifpirdylimahjqzraawgqojrnjujpluhrajwcdffnzydmddjpfceogrqwrtreuoevnficejsqltkcdxvapdnyflpragxfldcpgpsjkdkdomkysnrhjcjfdwxtttvlcalmejfhmtvzsagfxrrhzxcwzfghvhdzenzjyldwbbwxfpmilinvxxjx";
    char read_msg[1000];
    printf("Main: Trying to obtain TLS of size 1000...\n");
    ret = tls_create(1000000);
    assert(ret == 0);
    printf("...Main: Obtained\n\n");
    
    printf("Writing 1000 to main thread lsa\n");
    for(int i = 0; i < 1000; ++i){
        assert(tls_write(i*1000, 1000, o_msg) == 0);
    }
    printf("write 1000 to main sucessful\n");

    printf("launching 5 more threads...\n");

    for(int i = 0; i < 5; ++i){
        if(pthread_create(&alt_tid[i],NULL, start_routine, (void*)main_tid) == -1){
            printf("Unable to create thread\n");
        }
    }
    
    for(int i = 0; i < 5; ++i){
        assert(pthread_join(alt_tid[i],NULL)==0);
    }

    printf("Main: Trying to read back the message...\n");
    ret = tls_read(0, 1000, read_msg);
    assert(ret == 0);
    assert(strcmp(o_msg, read_msg) == 0);

    printf("Main: Trying to destroy my TLS...\n");
    ret = tls_destroy();
    assert(ret == 0);
    printf("...Main: Destroy successful\n");

    return 0;
}