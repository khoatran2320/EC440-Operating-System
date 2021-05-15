#include <stdio.h>
#include "../tls.h"
#include <time.h>
#include <assert.h>
#include <string.h>

//written by Khoa Tran with some remnants of Tony's tests

void *start_routine(void*tid){
    pthread_t main_tid = (pthread_t)tid;

    printf("Cloning main tls...\n");
    assert(tls_clone(main_tid) == 0);
    printf("Done cloning main tls...\n");

    char o_msg[1000] = "uqomukedhtycuzmlppwhpobwhinqyeisjqcjkfkefwstukpfhfhjmilsmjwfijvdrcfxkytptimphsbicrmuqjtofpxcujjspxcmzwtkdoyxiyihsqqemiujbquakwnkxuvnuipzwskppoffqajgqtmwatwtgfsvbidcnzylysqeixehiiwaoifthgemjslwelybsqjjwuqbkhjrzulpfalskyetgvswmmrsinzabdzcmzzfbiyzomoinnnvlzcbqhftnptslzfjuszeydlexdmseixjlsolhazkbxlxguihdhxsfjspbjoxmivwilqgscwkwtxcqwonghpepypnbmalbweqwimmibikhfdizvehzfmfcseklzshlduqvzigouftazxhvqiapiscprasvkftvjqibqgqewmojvwphmahyyqntfniaflqsstjgqyydqkttgqjyootfbbgkvkewqwcfmcsrkshmgyqmlptgsxpizkwtkvlcehsjdgziookdcwhqydtvdzqpesssulxerlpezykrkufmkxfcbuknywfaqcihoxzojgkeotxbqkokqkwnmjqwpnmfdrrukyhucxanjxvozjpjpffmboxlnbxbgqdvupauyoqwlctaexyluiosievoqjcessnyoocgwbkvohddmtpvezcqvjkneltbjlcpitrweuwwbxvqievbiipagshfhyvdkrcehlwjzyyeqmcjkbxwjaaralqtezrycomvcxjzianjwtfooiwipciermezesfpvivonuyudpygbzlqcpjgkpbaprilpozjkngbcifpirdylimahjqzraawgqojrnjujpluhrajwcdffnzydmddjpfceogrqwrtreuoevnficejsqltkcdxvapdnyflpragxfldcpgpsjkdkdomkysnrhjcjfdwxtttvlcalmejfhmtvzsagfxrrhzxcwzfghvhdzenzjyldwbbwxfpmilinvxxjx";

    char read_msg[1000];

    printf("Attempting to read after clone...\n");
    assert(tls_read(3096,1000, read_msg) == 0);
    assert(strcmp(o_msg, read_msg) == 0);
    printf("Read successful\n\n");

    char write_msg[500] = "tMaUgIgTfkWMQR95qjBdlZgb8CE1Iv2BXTv11DN8XiIR4tXnSDJZREv8mADzZJBzuBImsHFxAtLqbkpbBZjBiPc1gyJRIGnuZ5A7xkLFE1aFzaxCAmCq6LfVmKdtHK8kGorFTP1LURl89zqw1xlhdwh0lACm0l5BYOcN6cxnboRGOHLubw8MJ2HfwBKmE6kAjNp4CfbIY5te7cjwT6CBd6Jckd8UNtEvfA1aBSfwwCvddD95dZD64mhauenuHgGtyg6pJWPFnB7yML771WChEGzwkbC5IDojZiitzqsueJRPM2fMtbATRZ8XtwQOzQWKvyuybO0gxirHn2naxRBOtQUhKPN6jxc10almZgCgN1QzGn73b84d4C4HiNDlzTdxIkeyTwDvJMwAnijGgmxQEIhhS3LwEBcw2rqzm7USqBaV5S2lNQqa6SdhuozFkMhfF2FJ73stkYDjnBzEAvloyoHz5Sr7r2szQHnE4eSUL6g9a89htA3"; 
    printf("Attempting to write after cloning...\n");
    assert(tls_write(3700,500,write_msg)  == 0);
    memcpy(o_msg+500, write_msg, 500);
    char read1_msg[500];
    assert(tls_read(3700, 500, read1_msg) == 0);
    assert(strcmp(write_msg, read1_msg) == 0);
    printf("Write and read after clone successful\n");
    printf("thread exiting...\n\n");
    pthread_exit(NULL);
}
int main(int argc, char** argv) {

    pthread_t main_tid = pthread_self();
    pthread_t alt_tid;

    int ret;
    
    char o_msg[1000] = "uqomukedhtycuzmlppwhpobwhinqyeisjqcjkfkefwstukpfhfhjmilsmjwfijvdrcfxkytptimphsbicrmuqjtofpxcujjspxcmzwtkdoyxiyihsqqemiujbquakwnkxuvnuipzwskppoffqajgqtmwatwtgfsvbidcnzylysqeixehiiwaoifthgemjslwelybsqjjwuqbkhjrzulpfalskyetgvswmmrsinzabdzcmzzfbiyzomoinnnvlzcbqhftnptslzfjuszeydlexdmseixjlsolhazkbxlxguihdhxsfjspbjoxmivwilqgscwkwtxcqwonghpepypnbmalbweqwimmibikhfdizvehzfmfcseklzshlduqvzigouftazxhvqiapiscprasvkftvjqibqgqewmojvwphmahyyqntfniaflqsstjgqyydqkttgqjyootfbbgkvkewqwcfmcsrkshmgyqmlptgsxpizkwtkvlcehsjdgziookdcwhqydtvdzqpesssulxerlpezykrkufmkxfcbuknywfaqcihoxzojgkeotxbqkokqkwnmjqwpnmfdrrukyhucxanjxvozjpjpffmboxlnbxbgqdvupauyoqwlctaexyluiosievoqjcessnyoocgwbkvohddmtpvezcqvjkneltbjlcpitrweuwwbxvqievbiipagshfhyvdkrcehlwjzyyeqmcjkbxwjaaralqtezrycomvcxjzianjwtfooiwipciermezesfpvivonuyudpygbzlqcpjgkpbaprilpozjkngbcifpirdylimahjqzraawgqojrnjujpluhrajwcdffnzydmddjpfceogrqwrtreuoevnficejsqltkcdxvapdnyflpragxfldcpgpsjkdkdomkysnrhjcjfdwxtttvlcalmejfhmtvzsagfxrrhzxcwzfghvhdzenzjyldwbbwxfpmilinvxxjx";
    char read_msg[1000];
    printf("Main: Trying to obtain TLS of size 1000...\n");
    ret = tls_create(6000);
    assert(ret == 0);
    printf("...Main: Obtained\n\n");
    
    printf("Writing 1000 to main thread lsa\n");
    ret = tls_write(3096, 1000, o_msg);
    assert(ret == 0);
    printf("write 1000 to main sucessful\n");

    printf("launching second thread...\n");

    if(pthread_create(&alt_tid,NULL, start_routine, (void*)main_tid) == -1){
        printf("Unable to create thread\n");
    }
    pthread_join(alt_tid,NULL);

    printf("Main: Trying to read back the message...\n");
    ret = tls_read(3096, 1000, read_msg);
    assert(ret == 0);
    assert(strcmp(o_msg, read_msg) == 0);

    printf("Main: Trying to destroy my TLS...\n");
    ret = tls_destroy();
    assert(ret == 0);
    printf("...Main: Destroy successful\n");

    return 0;
}