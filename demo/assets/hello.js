
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



console.log(`run script end!!`);
