// Copyright 2020 by CrestoniX
#include <header.hpp>
#include <deque>
#include <boost/shared_ptr.hpp>
boost::asio::io_context IoContext;
boost::system::error_code error;
boost::recursive_mutex cs;

class talk_to_client
{
 public:
  explicit talk_to_client(const std::string& username)
      : sock_(IoContext), username_(username) {}
  using client_ptr = boost::shared_ptr<talk_to_client>;
  typedef std::vector<client_ptr> array;
  static array clients;

  std::string username() const { return username_; }

  void process_request()
  {
    bool found_enter = std::find(buff_, buff_ + already_read_,
                                 '\n') < buff_ + already_read_;
    if (!found_enter)
      return; // message is not full
    // process the msg
    last_ping = boost::posix_time::microsec_clock::local_time();
    size_t pos = std::find(buff_, buff_ + already_read_, '\n') - buff_;
    std::string msg(buff_, pos);
    std::copy(buff_ + already_read_, buff_ + max_msg, buff_);
    already_read_ -= pos + 1;
    if ( msg.find("login ") == 0) on_login(msg);
    else
    {
      if ( msg.find("ping") == 0) on_ping();
    }
    if ( msg.find("ask_clients") == 0) on_clients();
    else
      {
      std::cerr << "invalid msg " << msg << std::endl;
    }
  }

  void on_login(const std::string & msg)
  {
    std::istringstream in(msg);
    in >> username_ >> username_;
    write("login ok\n");
    set_clients_changed();
  }
  void write(const std::string& msg)
  {
    sock_.write_some(boost::asio::buffer(msg));
  }
  void set_clients_changed() {
    clients_changed_ = true;
  }

  void on_ping()
  {
    write(clients_changed_ ? "ping client_list_changed\n" : "ping ok\n");
    clients_changed_ = false;
  }

  void on_clients()
  {
    std::string msg;
    {
      boost::recursive_mutex::scoped_lock lk(cs);
      for (array::const_iterator b = clients.begin(),
                                 e = clients.end() ; b != e; ++b)
        msg += (*b)->username() + " ";
    }
    write("clients " + msg + "\n");
  }

  void answer_to_client() {
      try {
        read_request();
        process_request();
      } catch (boost::system::system_error&) {
        stop();
      }
      if (timed_out()) stop();
    }
    boost::asio::ip::tcp::socket& sock() { return sock_; }

    bool timed_out() const
    {
      boost::posix_time::ptime now =
          boost::posix_time::microsec_clock::local_time();
      int64_t ms = (now - last_ping).total_milliseconds();
      return ms > 5000;
    }

    void stop()
    {
      boost::system::error_code err;
      sock_.close(err);
    }

    void read_request()
    {
      if (sock_.available())
        already_read_ += sock_.read_some(
            boost::asio::buffer(buff_ + already_read_,
                                max_msg - already_read_));
    }


   private:
    boost::asio::ip::tcp::socket sock_;
    static const int max_msg = 1024;
    int already_read_;
    char buff_[1024];
    std::string username_;
    bool clients_changed_;
    boost::posix_time::ptime last_ping;
  };
std::vector<boost::shared_ptr<talk_to_client>> talk_to_client::clients;


void accept_thread() {
  boost::asio::ip::tcp::acceptor acceptor(IoContext,
                             boost::asio::ip::tcp::endpoint(
                                              boost::asio::ip::tcp::v4(),
                                              60013));
  while (true) {
    talk_to_client::client_ptr new_(new talk_to_client(""));
    acceptor.accept(new_->sock());
    boost::recursive_mutex::scoped_lock lk(cs);
    talk_to_client::clients.push_back(new_);
  }
}
  void handle_clients_thread() {
    while (true) {
      sleep(1);
      boost::recursive_mutex::scoped_lock lk(cs);
      std::vector<talk_to_client::client_ptr>::iterator e =
          talk_to_client::clients.end();
      for (std::vector<talk_to_client::client_ptr>::iterator b =
               talk_to_client::clients.begin();
           b != e; ++b)
        (*b)->answer_to_client();
      talk_to_client::clients.erase(
          std::remove_if(
              talk_to_client::clients.begin(), e,
              std::bind(&talk_to_client::timed_out, std::placeholders::_1)),
          e);
    }
  }
int main()
{
  boost::asio::detail::thread_group threads;
  threads.create_thread(accept_thread);
  threads.create_thread(handle_clients_thread);
  threads.join();
}









    // логгирование - начало
    /*std::recursive_mutex mutex;
    logging::add_file_log
    (
            keywords::file_name = "sample_%N.log",
            keywords::rotation_size = 10 * 1024 * 1024,
            keywords::time_based_rotation
             = sinks::file::rotation_at_time_point(0, 0, 0),
            keywords::format = "[%TimeStamp%]: %Message%");
    using logging::trivial::severity_level;
    using logging::trivial::severity_level::info;
    src::severity_logger<severity_level> lg;*/
    // логгирование - конец


