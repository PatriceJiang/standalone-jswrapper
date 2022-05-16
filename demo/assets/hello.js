
console.log(`run script hello.js`);



 let tank = new Tank2("Tiger");
 tank.fire(9, 1, 1);
 tank.id = 3333333;
 console.log(`set tank.id to 3333333, tank id ${tank.id}`);
 tank.readOnlyId = 909099090;
 console.log(`readonlyId after update, tank id ${tank.readOnlyId}`);
 tank.fire("U country");
 console.log(tank.numbers());
 
 if(tank.fire2) {
     console.log("Tank2 inherits Weapon");
     tank.fire2("J country");
 }else{
     console.error("!!! Tank2 no inherits Weapon");
 }

// let w = new Weapon(333,"Sdfs");
let w = new Weapon(333);
w.fire2(`fire2 power ${w.power()}`);
w.fire2(`fire2 power ${w.addid(10000000)}`);


let tank3 = new Tank2(8890, "Lion");
tank3.fire("E country");

tank.load(w);

Tank2.sleep(123456);


Tank2.rand = 33;
for(let i = 0;  i < 10 ;i ++) {
    tank3.nick = ""+i;
    console.log(` ${i} random value ${Tank2.rand} ${tank3.nick}`);
}


let t3 = new Tank3;
t3.hello;
t3.walk();
Tank3.walk2();
Tank3.hello2;
t3.hello = 333;
Tank3.hello = 4555;

console.log(`run script end!!`);


