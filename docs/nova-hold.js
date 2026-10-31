class NovaHold extends AudioWorkletProcessor{
  static get parameterDescriptors(){return [{name:'hold',defaultValue:1,minValue:1,maxValue:64,automationRate:'k-rate'}]}
  constructor(){super();this.ph=0;this.v=[0,0]}
  process(inputs,outputs,params){
    const inp=inputs[0],out=outputs[0],h=params.hold[0],step=1/h;
    if(!inp||!inp.length){for(const ch of out)ch.fill(0);return true}
    for(let i=0;i<out[0].length;i++){
      this.ph+=step;const take=h<=1.001||this.ph>=1;if(this.ph>=1)this.ph-=1;
      for(let c=0;c<out.length;c++){if(take)this.v[c]=(inp[c]||inp[0])[i];out[c][i]=this.v[c]}
    }
    return true;
  }
}
registerProcessor('nova-hold',NovaHold);
