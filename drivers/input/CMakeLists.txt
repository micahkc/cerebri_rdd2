if(CONFIG_RDD2_LOCKSTEP)
  target_sources(app PRIVATE lockstep_rc.c)
endif()

if(CONFIG_RDD2_RC_SYNAPSE)
  target_sources(app PRIVATE synapse_rc.c)
endif()
