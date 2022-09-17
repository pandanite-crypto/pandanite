#include "../core/crypto.hpp"
#include "../core/program.hpp"
#include "../server/program_store.hpp"
using namespace std;

TEST(test_program_db_stores_program) {
    ProgramStore pdb;
    
    vector<uint8_t> byteCode;
    byteCode.push_back(255);
    
    Program p(byteCode);

    ASSERT_EQUAL(p.getId(), SHA256((char*)byteCode.data(), byteCode.size()));
    
    pdb.init("./test-data/tmpdb");
    
    
    ASSERT_EQUAL(pdb.hasProgram(p.getId()), false);
    pdb.insertProgram(p);
    ASSERT_EQUAL(pdb.hasProgram(p.getId()), true);
    Program p2 = pdb.getProgram(p.getId());
    ASSERT_TRUE(p == p2);
    pdb.removeProgram(p);
    ASSERT_EQUAL(pdb.hasProgram(p.getId()), false);
}
