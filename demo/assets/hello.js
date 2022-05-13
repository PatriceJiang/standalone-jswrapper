
console.log(`run script hello.js`);


//let tank = new jswar.Tank("Tiger");
//tank.fire(9, 1, 1);
//

 let tank = new Tank2("Tiger");
 tank.fire(9, 1, 1);
 tank.id = 3333333;
 console.log(`tank id ${tank.id}`);
 tank.readOnlyId = 909099090;
 console.log(`tank id ${tank.readOnlyId}`);
 tank.fire("USA");
 
 if(tank.fire2) {
     console.log("Tane2 inherits Weapon");
     tank.fire2("Japaness");
 }else{
     console.log("Tank2 no inherits Weapon");
 }

// let w = new Weapon(333,"Sdfs");
let w = new Weapon(333);
w.fire2(`sdfsfd power ${w.power()}`);
w.fire2(`sdfsfd power ${w.addid(10000000)}`);


let tank3 = new Tank2(8890, "Lion");
tank3.fire("England");

tank.load(w);

Tank2.sleep(123456);


Tank2.rand = 33;
for(let i = 0;  i < 10 ;i ++) {
    tank3.nick = ""+i;
    console.log(` ${i} random value ${Tank2.rand} ${tank3.nick}`);
}


console.log(`run script end!!`);


