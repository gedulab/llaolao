cmd_/home/geduer/llaolao/Module.symvers := sed 's/\.ko$$/\.o/' /home/geduer/llaolao/modules.order | scripts/mod/modpost     -o /home/geduer/llaolao/Module.symvers -e -i Module.symvers   -T -
