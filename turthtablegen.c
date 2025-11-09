

#include <stdio.h>
int main(){
    int prefrence;
    printf("Choose an option for truth table generation:\n");
    pritf("1. Entire Truth Table\n");
    printf("2. Only for my entered values\n");
    scanf("%d",&prefrence);
    if(prefrence==1){   
        //code for entire truth table
    }
    else if(prefrence==2){
        //code for user entered values
    }
    else{
        printf("Invalid Option Selected\n");
    }
}
int fulltruthtablegen(int choice[number_of_gates], int number_of_gates){
    for(int i = 0; i < number_of_gates; i++) {
        // Generate and print the full truth table for each gate
        printf("Truth Table for Gate %d (%s):\n", i + 1, get_gate_name(choice[i]));
        // Implementation of truth table generation goes here
        switch(choice[i]) {
            case 1:
                // Generate AND gate truth table
                printf("Truth Table for AND Gate:\n");
                printf("A B | Output\n");
                printf("------------\n");
                printf("0 0 |   0\n");
                printf("0 1 |   0\n");
                printf("1 0 |   0\n");
                printf("1 1 |   1\n");
                break;
            case 2:
                // Generate OR gate truth table
                printf("Truth Table for OR Gate:\n");
                printf("A B | Output\n");
                printf("------------\n");
                printf("0 0 |   0\n");
                printf("0 1 |   1\n");
                printf("1 0 |   1\n");
                printf("1 1 |   1\n");
                break;
            case 3:
                // Generate NOT gate truth table
                printf("Truth Table for NOT Gate:\n");
                printf("A | Output\n");
                printf("----------\n");
                printf("0 |   1\n");
                printf("1 |   0\n");
                break;
            case 4:
                // Generate NAND gate truth table
                printf("Truth Table for NAND Gate:\n");
                printf("A B | Output\n");
                printf("------------\n");
                printf("0 0 |   1\n");
                printf("0 1 |   1\n");
                printf("1 0 |   1\n");
                printf("1 1 |   0\n");
                break;
            case 5:
                // Generate NOR gate truth table
                printf("Truth Table for NOR Gate:\n");
                printf("A B | Output\n");
                printf("------------\n");
                printf("0 0 |   1\n");
                printf("0 1 |   0\n");
                printf("1 0 |   0\n");
                printf("1 1 |   0\n");
                break;
            case 6:
                // Generate XOR gate truth table
                printf("Truth Table for XOR Gate:\n");
                printf("A B | Output\n");
                printf("------------\n");
                printf("0 0 |   0\n");
                printf("0 1 |   1\n");
                printf("1 0 |   1\n");
                printf("1 1 |   0\n");
                break;
            default:
                printf("Invalid gate choice!\n");
        }
    }
}

// Function for user-defined truth table generation
int userdefinedtruthtablegen(int choice[number_of_gates], int number_of_gates, int a, int b){
    for(int i = 0; i < number_of_gates; i++) {
        int result;
        printf("Calculating output for Gate %d (%s) with inputs A=%d, B=%d:\n", i + 1, get_gate_name(choice[i]), a, b);
        switch(choice[i]) {
            case 1:
                result = AND(a, b);
                printf("AND(%d, %d) = %d\n", a, b, result);
                break;
            case 2:
                result = OR(a, b);
                printf("OR(%d, %d) = %d\n", a, b, result);
                break;
            case 3:
                result = NOT(a);
                printf("NOT(%d) = %d\n", a, result);
                break;
            case 4:
                result = NAND(a, b);
                printf("NAND(%d, %d) = %d\n", a, b, result);
                break;
            case 5:
                result = NOR(a, b);
                printf("NOR(%d, %d) = %d\n", a, b, result);
                break;
            case 6:
                result = XOR(a, b);
                printf("XOR(%d, %d) = %d\n", a, b, result);
                break;
            default:
                printf("Invalid gate choice!\n");
        }
    }
}