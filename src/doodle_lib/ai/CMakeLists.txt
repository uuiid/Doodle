target_sources(
        doodle_lib
        PUBLIC 
        classifier_free_guided_model.h
        hf_tokenizer.h
        llm2vec_tokenizer.h
        llm2vec.h
        linear_layer.h
        positional_encoding.h
        timestep_embedder.h
        transformer_encoder_block.h
        twostage_denoiser.h

        
        PRIVATE
        classifier_free_guided_model.cpp
        hf_tokenizer.cpp
        llm2vec_tokenizer.cpp
        llm2vec.cpp
        linear_layer.cpp
        timestep_embedder.cpp
        transformer_encoder_block.cpp
        twostage_denoiser.cpp
)