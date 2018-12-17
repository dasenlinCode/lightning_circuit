#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <boost/optional/optional_io.hpp>

#include "snark.hpp"
#include "test.h"

using namespace libsnark;
using namespace std;

int main()
{
    double dur;
    clock_t start = clock();

    // Initialize the curve parameters.
    default_r1cs_ppzksnark_pp::init_public_params();
    // Generate the verifying/proving keys. (This is trusted setup!)
    auto keypair = generate_keypair<default_r1cs_ppzksnark_pp>();

    ofstream fileOut;
    //保存vk到文件
    stringstream verificationKey;
    verificationKey << keypair.vk;
    fileOut.open("vk");
    fileOut << verificationKey.rdbuf();
    fileOut.close();

    //保存pk到文件
    stringstream provingKey;
    provingKey << keypair.pk;
    fileOut.open("pk");
    fileOut << provingKey.rdbuf();
    fileOut.close();

    // 从文件中导入pk用于生成证明
     r1cs_ppzksnark_proving_key<default_r1cs_ppzksnark_pp> provingKey_in;
     ifstream fileIn("pk");  //from file of pk read the data to type of  r1cs_ppzksnark_proving_key
     stringstream provingKeyFromFile;
     if (fileIn) {
        provingKeyFromFile << fileIn.rdbuf();
        fileIn.close();
     }
     // provingKey_in == keypair.pk
     provingKeyFromFile >> provingKey_in;


    dur = (double)(clock() - start);
    printf("Generate keypair Use Time:%f\n\n",(dur/CLOCKS_PER_SEC));

    // Run test vectors.
    //assert(run_test(keypair, false, false, false));

    if(run_test(keypair, false, false, false)){
    	cout<<"verify succ"<<endl;
    }else{
    	cout<<"verify fail"<<endl;
    }
    //assert(!run_test(keypair, true, false, false));
    //assert(!run_test(keypair, false, true, false));
    //assert(!run_test(keypair, false, false, true));
}

bool run_test(r1cs_ppzksnark_keypair<default_r1cs_ppzksnark_pp>& keypair,
              // These are just for changing behavior
              // for testing purposes:
              bool use_and_instead_of_xor,
              bool swap_r1_r2,
              bool goofy_verification_inputs
    ) {

    // Initialize bit_vectors for all of the variables involved.
    std::vector<bool> h1_bv(256);
    std::vector<bool> h2_bv(256);
    std::vector<bool> h3_bv(256);
    std::vector<bool> r1_bv(256);
    std::vector<bool> r2_bv(256);
    std::vector<bool> r3_bv(256);

    {
        // These are working test vectors.
        h1_bv = int_list_to_bits({78,152,
  23,
  135,
  180,
  61,
  171,
  123,
  58,
  147,
  215,
  200,
  83,
  7,
  198,
  244,
  58,
  26,
  58,
  88,
  150,
  57,
  69,
  185,
  62,
  165,
  253,
  53,
  112,
  69, 80, 23}, 8);
        h2_bv = int_list_to_bits({182,
  169,
  95,
  91,
  248,
  154,
  156,
  163,
  104,
  18,
  251,
  174,
  68,
  251,
  237,
  249,
  215,
  166,
  135,
  222,
  50,
  133,
  48,
  197,
  197,
  205,
  182,
  20,
  56,
  166,
  108,
  66}, 8);
        h3_bv = int_list_to_bits({101,
  119,
  48,
  144,
  165,
  169,
  249,
  100,
  249,
  74,
  13,
  126,
  39,
  34,
  64,
  47,
  238,
  173,
  29,
  72,
  31,
  203,
  7,
  100,
  179,
  20,
  220,
  66,
  172,
  97,
  252,
  223}, 8);
        // r = (num, salt)
        // Constraint is num3 = num1 + num2
        r1_bv = int_list_to_bits({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 203, 6, 191, 16, 141, 210, 73, 136, 65, 136, 152, 60, 117, 24, 101, 18}, 8);
        r2_bv = int_list_to_bits({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 6, 178, 210, 43, 243, 10, 217, 251, 246, 248, 0, 21, 86, 194, 100, 94}, 8);
        r3_bv = int_list_to_bits({0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 203, 6, 191, 16, 141, 210, 73, 136, 65, 136, 152, 60, 117, 24, 101, 18}, 8);
    }

    if (swap_r1_r2) {
        // This swaps r1 and r2 which should test if the hashing
        // constraints work properly.
        auto tmp = r2_bv;
        r2_bv = r1_bv;
        r1_bv = tmp;
    }


    double dur;
    clock_t start = clock();

    cout << "Trying to generate proof..." << endl;
    auto proof = generate_proof<default_r1cs_ppzksnark_pp>(keypair.pk, h1_bv, h2_bv, h3_bv, r1_bv, r2_bv, r3_bv);
    cout << "Proof generated!" << endl;
    cout << "Proof: " <<  proof << endl;

    //保存证明数据
    stringstream proofStream;
    proofStream << *proof;
    ofstream fileOut;
    fileOut.open("proof");
    fileOut << proofStream.rdbuf();
    fileOut.close();


    dur = (double)(clock() - start);
    printf("Proof Generated Use Time:%f\n\n",(dur/CLOCKS_PER_SEC));
    start = clock();

    //读入证明数据
    r1cs_ppzksnark_proof<default_r1cs_ppzksnark_pp> proof_in;
    ifstream fileIn("proof");
    stringstream proofFromFile;
    if (fileIn) {
    	proofFromFile << fileIn.rdbuf();
    	fileIn.close();
    }
    proofFromFile >> proof_in;

    if (!proof) {
        return false;
    } else {
        if (goofy_verification_inputs) {
            // [test] if we generated the proof but try to validate
            // with bogus inputs it shouldn't let us
            return verify_proof(keypair.vk, *proof, h2_bv, h1_bv, h3_bv);
        } else {
            // verification should not fail if the proof is generated!
        	assert(verify_proof(keypair.vk, proof_in, h1_bv, h2_bv, h3_bv));
            //assert(verify_proof(keypair.vk, *proof, h1_bv, h2_bv, h3_bv));
            dur = (double)(clock() - start);
            printf("Proof Verify Use Time:%f\n\n",(dur/CLOCKS_PER_SEC));
            return true;
        }
    }
}
