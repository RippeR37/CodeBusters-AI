# RippeR's CodeBusters AI

C++11 implementation of AI bot for [CodeBusters](https://www.codingame.com/multiplayer/bot-programming/codebusters) game.


## Implemented game features

* Object-oriented implementation
* Support (and strategy) for all in-game commands
  * Including RADAR and EJECT
* Modular architecture for _ease of implementing new features_
* Extensive in-game data tracking
  * This covers almost everything that than be learned from both current and past game input data (like position of out-of-scope ghosts, STUN usage by both own and enemy busters etc.)
* Cooperation between busters


## Architecture

Each time something happens in-game, new task is created to handle it. Each round all combinations of `(buster, task)` are pushed to scoring system giving us an assignemnt. Assignment is a tuple of `(task, buster, score)` where:

  * `buster` specifies which buster should execute given task,
  * `task` specifies what should be done,
  * `score` describes how good is assignment of this given task to given buster.

When all tasks are processed, selectie algorithm kicks in and decides which assignments should be chosen for which busters (taking into consideration other assignments and any pending ones from previous rounds).

Available tasks are:

* `BUST` - attack given ghost and try to caputre it if in range,
* `STUN` - attack given enemy buster (with possibility to track it until stun cooldown ends),
* `COVER` - cover team member who carries a ghost to a base,
* `EXPLORE` - explore map (taking into account out-of-scope ghosts and map symmetry),
* `RETURN` - return to base with a ghost (possibly passing ghost to other buster),
* `RADAR` - use radar to scan map for ghosts.

Each task can have multiple stages of execution and each stage corresponds to in-game command like `BUST`, `MOVE` or `RELEASE`.


## Bot's successes

* reached **gold** league in first play,
* reached ~100 position in gold leauge,
* might have a potential to reach **legendary** league after tweaking factors.


## To do

* strategy for stealing ghosts from enemy base,
* tweaking contants and factors to.


## License

    You are free to study and learn from published code, but you can't directly or indirectly 
    use it in CodeBusters contest.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
    BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
