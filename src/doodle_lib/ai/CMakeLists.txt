target_sources(
        doodle_lib
        PUBLIC 
        hf_tokenizer.h
        llm2vec_tokenizer.h
        llm2vec.h

        
        PRIVATE
        hf_tokenizer.cpp
        llm2vec_tokenizer.cpp
        llm2vec.cpp
)