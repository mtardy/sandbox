#include <stdio.h>
#include <setjmp.h>

enum jb_t {
    j_function,
    j_loop,
    j_switch,
};

struct jb_record
{
    jmp_buf   jb;
    enum jb_t type;
    oop       result;
    struct jb_record next;
};

struct jb_record *jbs= 0;


int eval(oop ast)
{
    case t_call: {
    pushJbRec();
    if (0 != set_jmp(jbs->jb)) {
        oop result = jbs->result;
        popJbRec();
        return result;
    }
    // run the body of the function here
    result = (each statement in the func body...);
    popJbRec();
    return result;
    }
    setjmp(jb);
    if (n < 2) return 1;
    return 1 + f(n-1) + f(n-2);
}


int main()
{
    printf("%zi\n", sizeof(jmp_buf));
    printf("%i\n", f(5));
}
